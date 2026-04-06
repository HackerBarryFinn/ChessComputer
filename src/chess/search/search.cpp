#include "chess/search/search.h"

#include "chess/core/movegen.h"
#include "chess/rules/makemove.h"
#include "chess/search/eval.h"
#include "chess/rules/attacks.h"
#include "chess/core/utils.h"
#include "chess/search/tt.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <vector>
#include <array>

static constexpr int MATE_SCORE = 100000;
static constexpr int MATE_THRESHOLD = 90000;


/**
 * Adjusts a given evaluation score to be transposition-table compatible,
 * considering the ply depth to preserve mate distances.
 *
 * If the score indicates a mate situation (greater than MATE_THRESHOLD or less
 * than -MATE_THRESHOLD), it adds or subtracts the ply value to ensure correct
 * mate distance adjustments in transposition table storage. Non-mate scores
 * remain unaltered.
 *
 * @param score The original evaluation score to be adjusted.
 * @param ply   The current search depth in half-moves (plies).
 * @return The adjusted score for use in the transposition table.
 */
static inline int toTTScore(int score, int ply) {
    if (score > MATE_THRESHOLD)  return score + ply;
    if (score < -MATE_THRESHOLD) return score - ply;
    return score;
}

/**
 * Converts a transposition table score back to a standard evaluation score,
 * taking the ply depth into account to ensure correct mate distance
 * interpretation.
 *
 * If the score indicates a mate situation (greater than MATE_THRESHOLD or less
 * than -MATE_THRESHOLD), it adjusts the score by subtracting or adding the ply
 * value, respectively. Non-mate scores are returned unaltered.
 *
 * @param score The transposition table score to be converted.
 * @param ply   The current search depth in half-moves (plies).
 * @return The original evaluation score adjusted from the transposition table format.
 */
static inline int fromTTScore(int score, int ply) {
    if (score > MATE_THRESHOLD)  return score - ply;
    if (score < -MATE_THRESHOLD) return score + ply;
    return score;
}

/**
 * Represents the global transposition table used to store previously evaluated
 * positions and their associated search information to improve the efficiency
 * of the negamax search algorithm.
 *
 * The transposition table is a hash table that maps Zobrist keys (unique for
 * each board state) to entries containing the evaluation results, best moves,
 * and metadata. By caching search results, it avoids redundant calculations
 * for positions that have already been explored.
 *
 * This instance is initialized with a default size of 1,048,576 entries,
 * allocated as a power of 2 for efficient addressing via bit masking.
 *
 * It is accessed and updated throughout the search process to probe for
 * existing evaluations, save new results, and retrieve optimal moves.
 */
static TranspositionTable gTT(1u << 20);

// MVV-LVA Werte (Centipawns, König als 0)
static constexpr int PIECE_V[6] = {100, 320, 330, 500, 900, 0};

/**
 * Generates a bitboard with a single bit set at the given square.
 *
 * @param sq The square index (0 to 63) to set the bit for.
 * @return A 64-bit integer with only the bit at the specified square set.
 */
static inline uint64_t sqBB(int sq) { return 1ULL << sq; }

namespace {
    constexpr int MAX_PLY = 64;

    // zwei Killer pro Ply
    std::array<std::array<Move, 2>, MAX_PLY> gKillers{};
    // History: [sideToMove][from][to]
    std::array<std::array<std::array<int, 64>, 64>, 2> gHistory{};

    inline void resetKillerHistory() {
        for (int ply = 0; ply < MAX_PLY; ++ply) {
            gKillers[ply][0] = Move{};
            gKillers[ply][1] = Move{};
        }
        for (int s = 0; s < 2; ++s) {
            for (int from = 0; from < 64; ++from) {
                for (int to = 0; to < 64; ++to) {
                    gHistory[s][from][to] = 0;
                }
            }
        }
    }

    inline bool sameMoveKey(const Move& a, const Move& b) {
        return a.from == b.from && a.to == b.to && a.promotion == b.promotion;
    }

    inline bool isCaptureLike(const Move& m) {
        return (m.flags & CAPTURE) != 0;
    }

    inline void storeKiller(int ply, const Move& m) {
        if (ply < 0 || ply >= MAX_PLY) return;

        // keine Duplikate
        if (sameMoveKey(gKillers[ply][0], m)) return;

        gKillers[ply][1] = gKillers[ply][0];
        gKillers[ply][0] = m;
    }

    inline void addHistory(Color side, const Move& m, int depth) {
        if (m.from < 0 || m.from >= 64 || m.to < 0 || m.to >= 64) return;
        const int bonus = depth * depth; // klassisch, stabil
        int& entry = gHistory[side][m.from][m.to];

        // einfacher Clamp, damit es nicht unendlich wächst
        entry += bonus;
        if (entry > 1'000'000) entry = 1'000'000;
    }

    struct NullUndoState {
        Color prevSideToMove;
        uint64_t prevEnPassantTarget;
    };

    inline void makeNullMove(Board& board, NullUndoState& u) {
        u.prevSideToMove = board.sideToMove;
        u.prevEnPassantTarget = board.enPassantTarget;

        // Null-Move: Seite wechseln, EP löschen (EP-Rechte verfallen nach einem "Zug")
        board.enPassantTarget = 0ULL;
        board.sideToMove = (board.sideToMove == WHITE) ? BLACK : WHITE;
    }

    inline void unmakeNullMove(Board& board, const NullUndoState& u) {
        board.sideToMove = u.prevSideToMove;
        board.enPassantTarget = u.prevEnPassantTarget;
    }
}

/**
 * Evaluates the position of the chess board and returns the evaluation score
 * for the player whose turn it currently is.
 *
 * This method determines the evaluation score using the `evaluate` function
 * and adjusts the sign based on the side to move. A positive score favors
 * White, and a negative score favors Black.
 *
 * @param board The chess board state to evaluate, including piece positions
 *              and the side to move.
 * @return The evaluation score, positive if it favors the side to move,
 *         and negative if it disfavors the side to move.
 */
static int evalForSideToMove(const Board& board) {
    int s = evaluate(board);
    return (board.sideToMove == WHITE) ? s : -s;
}

/**
 * Determines the type of piece captured on the destination square of a move.
 *
 * This method identifies the type of piece captured, either through an en passant
 * capture or a standard capture. For en passant moves, the captured piece is
 * always a pawn. For standard captures, it checks the destination square to
 * identify the piece type based on the board's bitboards for the opponent's pieces.
 *
 * @param board The current game state, including piece positions and attributes.
 * @param m     The move to be analyzed, including the flags and destination square.
 * @return The type of piece captured (PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING)
 *         or -1 if no piece was captured.
 */
static int findCapturedPieceTypeOnToSquare(const Board& board, const Move& m) {
    // EP ist speziell: Opfer ist immer Pawn
    if (m.flags & EN_PASSANT) return PAWN;

    // Bei normalen Captures steht das Opfer auf m.to
    if ((m.flags & CAPTURE) == 0) return -1;

    Color stm = board.sideToMove;
    Color enemy = (stm == WHITE) ? BLACK : WHITE;
    uint64_t mask = sqBB(m.to);

    for (int pt = PAWN; pt <= KING; ++pt) {
        if (board.bitboards[enemy][pt] & mask) return pt;
    }
    return -1;
}

/**
 * Computes a "Most Valuable Victim, Least Valuable Attacker" (MVV-LVA) score
 * for a move based on the relative values of the captured and attacking pieces.
 *
 * For capture moves, the score favors moves where the captured piece (victim) is
 * more valuable and the attacking piece (attacker) is less valuable. Non-capture
 * moves are assigned a score of zero. The score can be used for move ordering
 * in search algorithms.
 *
 * @param board The current game board, providing access to piece locations and sides.
 * @param m     The move for which the MVV-LVA score is calculated.
 * @return The MVV-LVA score of the move, where higher scores represent
 *         more favorable captures.
 */
static int mvvLvaScore(const Board& board, const Move& m) {
    if ((m.flags & CAPTURE) == 0) return 0;

    int victim = (m.captured != -1) ? m.captured : findCapturedPieceTypeOnToSquare(board, m);
    if (victim < 0 || victim > KING) return 0;

    int attacker = m.moved;
    // „Most valuable victim“ hoch, „least valuable attacker“ niedrig
    return PIECE_V[victim] * 10 - PIECE_V[attacker];
}

/**
 * Orders a list of moves using the Most Valuable Victim - Least Valuable Attacker (MVV-LVA) heuristic.
 *
 * The MVV-LVA heuristic prioritizes captures based on the relative value of pieces,
 * favoring moves that capture higher-value pieces with lower-value pieces.
 * The list of moves is sorted in descending order of their MVV-LVA scores to facilitate
 * efficient move ordering in search algorithms.
 *
 * @param board The current state of the chess board.
 * @param moves The list of moves to be ordered by MVV-LVA priority.
 */
static void orderMovesMvvLva(const Board& board, std::vector<Move>& moves) {
    std::ranges::stable_sort(moves, [&](const Move& a, const Move& b) {
        return mvvLvaScore(board, a) > mvvLvaScore(board, b);
    });
}

/**
 * Orders a given list of moves based on a combination of factors, including
 * transposition table move bonus, capture prioritization, killer moves,
 * and history heuristic scores. This aids in move ordering for more efficient
 * search by prioritizing promising moves.
 *
 * @param board         The current game board state.
 * @param moves         The list of moves to be ordered.
 * @param ply           The current search depth in half-moves (plies).
 * @param ttMoveOrEmpty The move retrieved from the transposition table, or an
 *                      empty move if no such move exists.
 */
static void orderMovesKillerHistory(const Board& board,
                                    std::vector<Move>& moves,
                                    int ply,
                                    const Move& ttMoveOrEmpty) {
    const Color stm = board.sideToMove;

    // große Abstände, damit Prioritäten klar bleiben
    static constexpr int TT_BONUS      = 5'000'000;
    static constexpr int CAPTURE_BASE  = 1'000'000;
    static constexpr int KILLER1_BONUS = 900'000;
    static constexpr int KILLER2_BONUS = 800'000;

    auto score = [&](const Move& m) -> int {
        int s = 0;

        if (ttMoveOrEmpty.from != 0 || ttMoveOrEmpty.to != 0 || ttMoveOrEmpty.flags != 0 || ttMoveOrEmpty.promotion != -1) {
            if (sameMoveKey(m, ttMoveOrEmpty)) s += TT_BONUS;
        }

        if (isCaptureLike(m)) {
            s += CAPTURE_BASE + mvvLvaScore(board, m);
            return s;
        }

        if (ply >= 0 && ply < MAX_PLY) {
            if (sameMoveKey(m, gKillers[ply][0])) s += KILLER1_BONUS;
            else if (sameMoveKey(m, gKillers[ply][1])) s += KILLER2_BONUS;
        }

        s += gHistory[stm][m.from][m.to];
        return s;
    };

    std::ranges::stable_sort(moves, [&](const Move& a, const Move& b) {
        return score(a) > score(b);
    });
}

/**
 * Performs a quiescence search to evaluate the position more accurately in
 * non-tactical scenarios, focusing only on capturing moves to resolve
 * drastic evaluation changes caused by moves like promotions or captures.
 *
 * The search evaluates the static position score and recursively explores
 * capture moves. If the position is deemed unpromising (alpha-beta cutoff),
 * it terminates early, improving efficiency.
 *
 * @param board The current state of the chessboard.
 * @param alpha The lower bound of the alpha-beta window, representing the
 *              best score that the maximizing player is assured of.
 * @param beta  The upper bound of the alpha-beta window, representing the
 *              worst score that the minimizing player is willing to consider.
 * @return The evaluation score for the position considering capture moves
 *         within the search window.
 */
static int quiescence(Board& board, int alpha, int beta);

/**
 * Performs a depth-first negamax search with alpha-beta pruning optimizations
 * to evaluate and determine the best move in a given chess position. This method
 * incorporates techniques such as null-move pruning, late move reductions (LMR),
 * and transposition table probing for efficient move ordering and pruning.
 *
 * @param board          The chess board state to evaluate.
 * @param depth          The remaining search depth in plies.
 * @param alpha          The alpha bound for alpha-beta pruning.
 * @param beta           The beta bound for alpha-beta pruning.
 * @param ply            The current search depth in half-moves (plies), used for mate distance adjustments. Default is 0.
 * @param allowNullMove  Determines whether null-move pruning is allowed. Default is true.
 * @return The best evaluation score for the current position from the side to move's perspective.
 */
static int negamax(Board& board, int depth, int alpha, int beta, int ply = 0, bool allowNullMove = true) {
    const uint64_t key = computeZobrist(board);

    // TT Probe
    TTEntry hit;
    Move ttMove{};
    if (gTT.probe(key, hit) && hit.depth >= depth) {
        int ttScore = fromTTScore(hit.score, ply);

        if (hit.flag == TTFlag::EXACT)
            return ttScore;

        if (hit.flag == TTFlag::LOWERBOUND)
            alpha = std::max(alpha, ttScore);
        else if (hit.flag == TTFlag::UPPERBOUND)
            beta = std::min(beta, ttScore);

        if (alpha >= beta)
            return ttScore;

        ttMove = hit.bestMove;
    } else if (gTT.probe(key, hit)) {
        ttMove = hit.bestMove;
    }

    // Null-Move Pruning
    if (allowNullMove && depth >= 3 && ply < MAX_PLY - 1) {
        const Color side = board.sideToMove;
        const Color enemy = (side == WHITE) ? BLACK : WHITE;

        const int ksq = board.kingSq[side];
        const bool inCheck = (ksq != -1) && isSquareAttacked(board, ksq, enemy);

        if (!inCheck) {
            // Reduktion R (klassisch ~2). Für kleine Engine: fix 2 ist ok.
            constexpr int R = 2;

            if (depth > R + 1) {
                NullUndoState nu{};
                makeNullMove(board, nu);

                // Null-Window Search (fail-high Test)
                int score = -negamax(board,
                                     depth - 1 - R,
                                     -beta,
                                     -beta + 1,
                                     ply + 1,
                                     false);

                unmakeNullMove(board, nu);

                if (score >= beta) {
                    // fail-hard cutoff
                    return beta;
                }
            }
        }
    }

    auto moves = generateLegalMoves(board, board.sideToMove);

    // Terminal: Matt oder Patt
    if (moves.empty()) {
        Color side = board.sideToMove;
        Color enemy = (side == WHITE) ? BLACK : WHITE;
        int kingSq = findKingSquare(board, side);
        bool inCheck = (kingSq != -1) && isSquareAttacked(board, kingSq, enemy);

        int res = inCheck ? -(MATE_SCORE - ply) : 0;

        gTT.store(key, depth, toTTScore(res, ply), TTFlag::EXACT, Move{});
        return res;
    }

    // Blatt: Quiescence
    if (depth == 0) {
        int res = quiescence(board, alpha, beta);
        gTT.store(key, depth, toTTScore(res, ply), TTFlag::EXACT, Move{});
        return res;
    }

    // TT move > captures (MVV-LVA) > killer > history
    orderMovesKillerHistory(board, moves, ply, ttMove);

    const int originalAlpha = alpha;
    int bestScore = std::numeric_limits<int>::min();
    Move bestMove{};

    const Color stmNode = board.sideToMove;
    const Color enemyNode = (stmNode == WHITE) ? BLACK : WHITE;
    const int kingSqNode = board.kingSq[stmNode];
    const bool inCheckNode = (kingSqNode != -1) && isSquareAttacked(board, kingSqNode, enemyNode);

    int moveIndex = 0;
    for (const auto& m : moves) {
        ++moveIndex;

        UndoState u{};
        if (!makeMove(board, m, u))
            continue;

        int score = std::numeric_limits<int>::min();

        // Late Move Reductions (LMR)
        // Idee: „späte“ Züge (nach den guten Kandidaten) zunächst mit reduzierter Tiefe
        // in einem Null-Window suchen. Nur wenn sie Alpha verbessern könnten -> volle Suche.
        bool canLMR = false;
        if (depth >= 3 && !inCheckNode && !isCaptureLike(m) && moveIndex > 3) {
            bool isTT = (ttMove.from != 0 || ttMove.to != 0 || ttMove.flags != 0 || ttMove.promotion != -1)
                        && sameMoveKey(m, ttMove);

            bool isKiller = false;
            if (ply >= 0 && ply < MAX_PLY) {
                isKiller = sameMoveKey(m, gKillers[ply][0]) || sameMoveKey(m, gKillers[ply][1]);
            }

            if (!isTT && !isKiller) canLMR = true;
        }

        if (canLMR) {
            // einfache Reduktionsformel
            int R = 1;
            if (depth >= 5 && moveIndex > 6) R = 2;

            // Reduced-Depth Null-Window Search
            const int reducedDepth = (depth - 1 - R);
            if (reducedDepth > 0) {
                int reduced = -negamax(board, reducedDepth, -alpha - 1, -alpha, ply + 1, true);

                if (reduced > alpha) {
                    // Re-Search mit voller Tiefe + normalem Fenster
                    score = -negamax(board, depth - 1, -beta, -alpha, ply + 1, true);
                } else {
                    score = reduced;
                }
            } else {
                // falls Reduktion zu aggressiv wäre: normal suchen
                score = -negamax(board, depth - 1, -beta, -alpha, ply + 1, true);
            }
        } else {
            // normale Vollsuche
            score = -negamax(board, depth - 1, -beta, -alpha, ply + 1, true);
        }
        // ---------------------------------------------------------------

        unmakeMove(board, m, u);

        if (score > bestScore) {
            bestScore = score;
            bestMove = m;
        }

        if (score > alpha)
            alpha = score;

        if (alpha >= beta) {
            // Beta cutoff -> Killer/History nur für Quiet-Moves updaten
            if (!isCaptureLike(m)) {
                storeKiller(ply, m);
                addHistory(board.sideToMove, m, depth);
            }
            break;
        }
    }

    // TT Store
    TTFlag flag = TTFlag::EXACT;
    if (bestScore <= originalAlpha) flag = TTFlag::UPPERBOUND;
    else if (bestScore >= beta)     flag = TTFlag::LOWERBOUND;

    gTT.store(key, depth, toTTScore(bestScore, ply), flag, bestMove);

    return bestScore;
}

/**
 * Performs a quiescence search to evaluate a chess position beyond the current
 * depth limit, focusing on "quiet" positions where no immediate captures or
 * tactical threats are present. This method evaluates potential captures
 * within the position by recursively exploring only capturing moves, improving
 * the evaluation and decision-making process.
 *
 * The function begins by checking the static evaluation (stand-pat score) of
 * the board. If the static evaluation is already greater than or equal to
 * beta, the function immediately returns beta as a cutoff. Otherwise, it
 * adjusts alpha if the static evaluation is higher than alpha.
 *
 * Candidate moves are limited to captures and are ordered based on the
 * most-valuable-victim/least-valuable-attacker (MVV-LVA) heuristic for
 * efficient exploration. Each capturing move is made, and a recursive quiescence
 * search is performed on the resulting position. The search updates alpha or
 * returns beta if it encounters further cutoffs.
 *
 * @param board The current state of the chessboard.
 * @param alpha The alpha value used for alpha-beta pruning, representing
 *              the lower bound of possible score estimates.
 * @param beta  The beta value used for alpha-beta pruning, representing
 *              the upper bound of possible score estimates.
 * @return The best score found within the bounds of alpha and beta
 *         during the quiescence search.
 */
static int quiescence(Board& board, int alpha, int beta) {
    int standPat = evalForSideToMove(board);
    if (standPat >= beta) return beta;
    if (standPat > alpha) alpha = standPat;

    auto moves = generateLegalMoves(board, board.sideToMove);
    // Nur Captures
    std::erase_if(moves, [](const Move& m) {
        return (m.flags & CAPTURE) == 0;
    });

    orderMovesMvvLva(board, moves);

    for (const auto& m : moves) {
        UndoState u{};
        if (!makeMove(board, m, u)) continue;

        int score = -quiescence(board, -beta, -alpha);

        unmakeMove(board, m, u);

        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }

    return alpha;
}

/**
 * Evaluates a specific root move by performing it on the provided board,
 * calculating the resulting position's score using the negamax algorithm,
 * and then undoing the move to restore the board's original state.
 *
 * The move is scored from the perspective of the side to move, and the score
 * is computed by searching one ply less than the given depth to simulate
 * the positional evaluation of the next move.
 *
 * If the provided move is invalid, the function returns the minimum integer value
 * to represent an invalid or losing move.
 *
 * @param board The current game state represented as a board to evaluate the move on.
 * @param m     The move to be evaluated, including its origin, destination, and other metadata.
 * @param depth The remaining search depth in plies for the evaluation.
 * @return The computed score for the move from the perspective of the player to move.
 */
static int scoreRootMove(Board& board, const Move& m, int depth) {
    UndoState u{};
    if (!makeMove(board, m, u)) return std::numeric_limits<int>::min();

    int alpha = std::numeric_limits<int>::min() + 1;
    int beta  = std::numeric_limits<int>::max();

    int score = -negamax(board, depth - 1, -beta, -alpha, 1, true);

    unmakeMove(board, m, u);
    return score;
}

/**
 * Determines the best move for the current player by evaluating all legal moves
 * from a given board state up to a specified search depth.
 *
 * This method uses features like killer move history and root move ordering
 * to optimize move selection. It evaluates each legal move's score and selects
 * the move with the highest score.
 *
 * @param board The current state of the chessboard to analyze. Modifications
 *              to the board during analysis are reverted by the end of this method.
 * @param depth The maximum search depth (in plies) for evaluating moves.
 * @return The best move calculated based on the given board state and search depth.
 *         If no legal moves are available, an empty `Move` object is returned.
 */
Move findBestMove(Board& board, int depth) {
    resetKillerHistory();

    Move bestMove{};
    int bestScore = std::numeric_limits<int>::min();

    auto moves = generateLegalMoves(board, board.sideToMove);
    if (moves.empty()) return bestMove;

    // root ordering ebenfalls profitieren lassen
    orderMovesKillerHistory(board, moves, 0, Move{});

    for (const auto& m : moves) {
        int score = scoreRootMove(board, m, depth);
        if (score > bestScore) {
            bestScore = score;
            bestMove = m;
        }
    }

    return bestMove;
}

/**
 * Displays the top moves for the given board position up to a specified depth.
 * The moves are scored based on a root negamax evaluation and sorted by their
 * scores in descending order before being displayed. The number of top moves
 * displayed is limited by the parameter `topN`.
 *
 * @param board The current state of the chessboard, including piece positions,
 *              side to move, and game metadata such as castling rights.
 * @param depth The search depth in plies (half-moves) to evaluate the moves.
 *              A higher depth provides more accurate scoring but increases
 *              computation time.
 * @param topN  The number of top moves to display, sorted by score.
 *              If `topN` exceeds the number of legal moves, all legal moves
 *              are displayed. Values less than 1 default to 1.
 */
void printTopMoves(Board& board, int depth, int topN) {
    struct ScoredMove { Move m; int score; };
    std::vector<ScoredMove> scored;

    auto moves = generateLegalMoves(board, board.sideToMove);
    orderMovesMvvLva(board, moves);

    for (const auto& m : moves) {
        scored.push_back({m, scoreRootMove(board, m, depth)});
    }

    std::ranges::stable_sort(scored, [](const ScoredMove& a, const ScoredMove& b) {
        return a.score > b.score;
    });

    if (topN < 1) topN = 1;
    if (topN > static_cast<int>(scored.size())) topN = static_cast<int>(scored.size());

    std::cout << "Top " << topN << " Zuege @depth " << depth << " (Score aus Sicht sideToMove):\n";
    for (int i = 0; i < topN; ++i) {
        const auto& sm = scored[i];

        std::cout << "  " << (i + 1) << ") "
                  << squareToString(sm.m.from) << squareToString(sm.m.to);

        if (sm.m.flags & PROMOTION) std::cout << pieceToChar(sm.m.promotion);
        if (sm.m.flags & CASTLING) std::cout << " (castle)";
        if (sm.m.flags & CAPTURE) std::cout << "x";

        std::cout << "  score=" << sm.score << "\n";
    }
}

/**
 * Finds the best move using an iterative deepening approach up to a specified maximum depth.
 * This method initializes necessary search structures, generates legal moves,
 * orders them using heuristics, and iteratively evaluates moves to determine the optimal one.
 *
 * @param board    The current state of the chess board.
 * @param maxDepth The maximum search depth to evaluate in half-moves (plies).
 * @return The best move calculated based on the evaluation function.
 */
Move findBestMoveIterative(Board& board, int maxDepth) {
    // Für Iterative Deepening: TT NICHT leeren
    resetKillerHistory();

    Move best{};

    for (int d = 1; d <= maxDepth; ++d) {
        auto moves = generateLegalMoves(board, board.sideToMove);
        if (moves.empty()) break;

        orderMovesKillerHistory(board, moves, 0, Move{});

        Move localBest{};
        int localBestScore = std::numeric_limits<int>::min();

        for (const auto& m : moves) {
            int score = scoreRootMove(board, m, d);
            if (score > localBestScore) {
                localBestScore = score;
                localBest = m;
            }
        }

        best = localBest;
        int bestScore = localBestScore;

        std::cout << "ID depth " << d << ": bestmove "
                  << squareToString(best.from) << squareToString(best.to)
                  << " score=" << bestScore << "\n";
    }

    return best;
}
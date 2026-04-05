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

// Hilfsfunktionen für Mate-Distanzierung
static inline int toTTScore(int score, int ply) {
    if (score > MATE_THRESHOLD)  return score + ply;
    if (score < -MATE_THRESHOLD) return score - ply;
    return score;
}

static inline int fromTTScore(int score, int ply) {
    if (score > MATE_THRESHOLD)  return score - ply;
    if (score < -MATE_THRESHOLD) return score + ply;
    return score;
}

// Globale TT
static TranspositionTable gTT(1u << 20);

// MVV-LVA Werte (Centipawns, König als 0)
static constexpr int PIECE_V[6] = {100, 320, 330, 500, 900, 0};

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

static int evalForSideToMove(const Board& board) {
    int s = evaluate(board);
    return (board.sideToMove == WHITE) ? s : -s;
}

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

static int mvvLvaScore(const Board& board, const Move& m) {
    if ((m.flags & CAPTURE) == 0) return 0;

    int victim = (m.captured != -1) ? m.captured : findCapturedPieceTypeOnToSquare(board, m);
    if (victim < 0 || victim > KING) return 0;

    int attacker = m.moved;
    // „Most valuable victim“ hoch, „least valuable attacker“ niedrig
    return PIECE_V[victim] * 10 - PIECE_V[attacker];
}

static void orderMovesMvvLva(const Board& board, std::vector<Move>& moves) {
    std::ranges::stable_sort(moves, [&](const Move& a, const Move& b) {
        return mvvLvaScore(board, a) > mvvLvaScore(board, b);
    });
}

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

static int quiescence(Board& board, int alpha, int beta);

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

    // ------------------- Null-Move Pruning -------------------
    // Bedingungen (konservativ):
    // - nicht im Schach
    // - genügend Tiefe
    // - kein aufeinanderfolgender Null-Move
    // - kein "Mate-Score"-Bereich (optional, aber hier weggelassen)
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
    // ---------------------------------------------------------

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

    // Für LMR brauchen wir „inCheck“-Status am Knoten (konservativ)
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

        // ------------------- Late Move Reductions (LMR) -------------------
        // Idee: „späte“ Züge (nach den guten Kandidaten) zunächst mit reduzierter Tiefe
        // in einem Null-Window suchen. Nur wenn sie Alpha verbessern könnten -> volle Suche.
        //
        // Konservative Bedingungen:
        // - genügend Tiefe
        // - nicht im Schach (sonst Taktiken)
        // - Quiet-Move (keine Captures)
        // - nicht einer der ersten Moves
        // - nicht TT-Move / nicht Killer
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
            // einfache Reduktionsformel (robust für kleine Engines)
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

static int scoreRootMove(Board& board, const Move& m, int depth) {
    UndoState u{};
    if (!makeMove(board, m, u)) return std::numeric_limits<int>::min();

    int alpha = std::numeric_limits<int>::min() + 1;
    int beta  = std::numeric_limits<int>::max();

    int score = -negamax(board, depth - 1, -beta, -alpha, 1, true);

    unmakeMove(board, m, u);
    return score;
}

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
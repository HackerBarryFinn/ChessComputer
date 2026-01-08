#include "chess/search/search.h"

#include "chess/core/movegen.h"
#include "chess/rules/makemove.h"
#include "chess/search/eval.h"
#include "chess/rules/attacks.h"
#include "../../../include/app/utils.h"
#include "chess/search/tt.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <vector>

static constexpr int MATE_SCORE = 100000;

// Globale TT (später ggf. in Engine-Klasse kapseln)
static TranspositionTable gTT(1u << 20);

// MVV-LVA Werte (Centipawns, König als 0)
static constexpr int PIECE_V[6] = {100, 320, 330, 500, 900, 0};

static inline uint64_t sqBB(int sq) { return 1ULL << sq; }

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
    std::stable_sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
        return mvvLvaScore(board, a) > mvvLvaScore(board, b);
    });
}

static int quiescence(Board& board, int alpha, int beta);

static int negamax(Board& board, int depth, int alpha, int beta) {
    const uint64_t key = computeZobrist(board);

    // TT Probe
    TTEntry hit;
    if (gTT.probe(key, hit) && hit.depth >= depth) {
        if (hit.flag == TTFlag::EXACT) return hit.score;

        if (hit.flag == TTFlag::LOWERBOUND) alpha = std::max(alpha, hit.score);
        else if (hit.flag == TTFlag::UPPERBOUND) beta = std::min(beta, hit.score);

        if (alpha >= beta) return hit.score;
    }

    auto moves = generateLegalMoves(board, board.sideToMove);

    // Terminal: keine Züge -> Matt oder Patt
    if (moves.empty()) {
        Color side = board.sideToMove;
        Color enemy = (side == WHITE) ? BLACK : WHITE;
        int kingSq = findKingSquare(board, side);
        bool inCheck = (kingSq != -1) && isSquareAttacked(board, kingSq, enemy);

        int res = inCheck ? -MATE_SCORE : 0;
        gTT.store(key, depth, res, TTFlag::EXACT, Move{});
        return res;
    }

    // Blatt: Quiescence statt direkter Eval
    if (depth == 0) {
        int res = quiescence(board, alpha, beta);
        gTT.store(key, depth, res, TTFlag::EXACT, Move{});
        return res;
    }

    orderMovesMvvLva(board, moves);

    const int originalAlpha = alpha;
    int bestScore = std::numeric_limits<int>::min();
    Move bestMove{};

    for (const auto& m : moves) {
        UndoState u{};
        if (!makeMove(board, m, u)) continue;

        int score = -negamax(board, depth - 1, -beta, -alpha);

        unmakeMove(board, m, u);

        if (score > bestScore) {
            bestScore = score;
            bestMove = m;
        }

        if (score > alpha) alpha = score;
        if (alpha >= beta) break; // cutoff
    }

    // TT Store (Bound-Typ)
    TTFlag flag = TTFlag::EXACT;
    if (bestScore <= originalAlpha) flag = TTFlag::UPPERBOUND;
    else if (bestScore >= beta) flag = TTFlag::LOWERBOUND;

    gTT.store(key, depth, bestScore, flag, bestMove);
    return bestScore;
}

static int quiescence(Board& board, int alpha, int beta) {
    int standPat = evalForSideToMove(board);
    if (standPat >= beta) return beta;
    if (standPat > alpha) alpha = standPat;

    auto moves = generateLegalMoves(board, board.sideToMove);
    // Nur Captures
    moves.erase(std::remove_if(moves.begin(), moves.end(), [](const Move& m) {
        return (m.flags & CAPTURE) == 0;
    }), moves.end());

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

    int score = -negamax(board, depth - 1, -beta, -alpha);

    unmakeMove(board, m, u);
    return score;
}

Move findBestMove(Board& board, int depth) {
    Move bestMove{};
    int bestScore = std::numeric_limits<int>::min();

    auto moves = generateLegalMoves(board, board.sideToMove);
    if (moves.empty()) return bestMove;

    orderMovesMvvLva(board, moves);

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

    std::stable_sort(scored.begin(), scored.end(), [](const ScoredMove& a, const ScoredMove& b) {
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
    Move best{};
    int bestScore = std::numeric_limits<int>::min();

    for (int d = 1; d <= maxDepth; ++d) {
        auto moves = generateLegalMoves(board, board.sideToMove);
        if (moves.empty()) break;

        orderMovesMvvLva(board, moves);

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
        bestScore = localBestScore;

        std::cout << "ID depth " << d << ": bestmove "
                  << squareToString(best.from) << squareToString(best.to)
                  << " score=" << bestScore << "\n";
    }

    return best;
}
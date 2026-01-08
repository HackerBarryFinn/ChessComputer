#include "chess/search/eval.h"
#include <bit>

static int mirrorSquare(int sq) {
    int file = sq % 8;
    int rank = sq / 8;
    int mrank = 7 - rank;
    return mrank * 8 + file;
}

static int evalPstWhite(uint64_t bb, const int pst[64]) {
    int sum = 0;
    while (bb) {
        int sq = std::countr_zero(bb);
        bb &= (bb - 1);
        sum += pst[sq];
    }
    return sum;
}

static int evalPstBlack(uint64_t bb, const int pst[64]) {
    int sum = 0;
    while (bb) {
        int sq = std::countr_zero(bb);
        bb &= (bb - 1);
        sum += pst[mirrorSquare(sq)];
    }
    return sum;
}

int evaluate(const Board& board) {
    // Materialwerte
    constexpr int V[6] = {100, 320, 330, 500, 900, 0};

    // PSTs:
    // - Knights/Bishops: zentral + aktiv = gut
    // - Rooks: 7. Reihe leicht bevorzugt (in der Praxis wichtig), sonst neutral
    // - Queen: leicht zentral, aber nicht zu stark (sonst frühe Queen-Ausflüge)
    static constexpr int PAWN_PST[64] = {
         0,  0,  0,  0,  0,  0,  0,  0,
         5,  5,  5,  5,  5,  5,  5,  5,
         2,  2,  4,  6,  6,  4,  2,  2,
         1,  1,  3,  8,  8,  3,  1,  1,
         1,  1,  3, 10, 10,  3,  1,  1,
         2,  2,  4,  6,  6,  4,  2,  2,
         5,  5,  5,  5,  5,  5,  5,  5,
         0,  0,  0,  0,  0,  0,  0,  0
    };

    static constexpr int KNIGHT_PST[64] = {
        -5, -4, -3, -3, -3, -3, -4, -5,
        -4, -2,  0,  0,  0,  0, -2, -4,
        -3,  0,  2,  3,  3,  2,  0, -3,
        -3,  1,  3,  4,  4,  3,  1, -3,
        -3,  1,  3,  4,  4,  3,  1, -3,
        -3,  0,  2,  3,  3,  2,  0, -3,
        -4, -2,  0,  1,  1,  0, -2, -4,
        -5, -4, -3, -3, -3, -3, -4, -5
    };

    static constexpr int BISHOP_PST[64] = {
        -2, -1, -1, -1, -1, -1, -1, -2,
        -1,  0,  0,  0,  0,  0,  0, -1,
        -1,  0,  1,  2,  2,  1,  0, -1,
        -1,  1,  2,  3,  3,  2,  1, -1,
        -1,  1,  2,  3,  3,  2,  1, -1,
        -1,  0,  1,  2,  2,  1,  0, -1,
        -1,  0,  0,  0,  0,  0,  0, -1,
        -2, -1, -1, -1, -1, -1, -1, -2
    };

    static constexpr int ROOK_PST[64] = {
         0,  0,  0,  0,  0,  0,  0,  0,
         1,  1,  1,  2,  2,  1,  1,  1,
         0,  0,  0,  1,  1,  0,  0,  0,
         0,  0,  0,  1,  1,  0,  0,  0,
         0,  0,  0,  1,  1,  0,  0,  0,
         0,  0,  0,  1,  1,  0,  0,  0,
         3,  3,  3,  4,  4,  3,  3,  3, // 7. Reihe für Weiß (Rank 7) leicht bevorzugt
         0,  0,  0,  0,  0,  0,  0,  0
    };

    static constexpr int QUEEN_PST[64] = {
        -2, -1, -1, -1, -1, -1, -1, -2,
        -1,  0,  0,  0,  0,  0,  0, -1,
        -1,  0,  1,  1,  1,  1,  0, -1,
        -1,  0,  1,  2,  2,  1,  0, -1,
        -1,  0,  1,  2,  2,  1,  0, -1,
        -1,  0,  1,  1,  1,  1,  0, -1,
        -1,  0,  0,  0,  0,  0,  0, -1,
        -2, -1, -1, -1, -1, -1, -1, -2
    };

    int score = 0;

    // 1) Material
    for (int pt = PAWN; pt <= KING; ++pt) {
        int w = static_cast<int>(std::popcount(board.bitboards[WHITE][pt]));
        int b = static_cast<int>(std::popcount(board.bitboards[BLACK][pt]));
        score += V[pt] * (w - b);
    }

    // 2) PST Weiß
    score += evalPstWhite(board.bitboards[WHITE][PAWN],   PAWN_PST);
    score += evalPstWhite(board.bitboards[WHITE][KNIGHT], KNIGHT_PST);
    score += evalPstWhite(board.bitboards[WHITE][BISHOP], BISHOP_PST);
    score += evalPstWhite(board.bitboards[WHITE][ROOK],   ROOK_PST);
    score += evalPstWhite(board.bitboards[WHITE][QUEEN],  QUEEN_PST);

    // 3) PST Schwarz gespiegelt
    score -= evalPstBlack(board.bitboards[BLACK][PAWN],   PAWN_PST);
    score -= evalPstBlack(board.bitboards[BLACK][KNIGHT], KNIGHT_PST);
    score -= evalPstBlack(board.bitboards[BLACK][BISHOP], BISHOP_PST);
    score -= evalPstBlack(board.bitboards[BLACK][ROOK],   ROOK_PST);
    score -= evalPstBlack(board.bitboards[BLACK][QUEEN],  QUEEN_PST);

    return score;
}
#include "chess/search/eval.h"
#include <bit>

static inline uint64_t sqBB(int sq) { return 1ULL << sq; }

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

static int pawnAdvanceBonusWhite(uint64_t pawns) {
    // Bonus je weiter vorne (Rank 2..7). Sehr simpel, aber hilft bei Promotion-Plan.
    // rank = sq/8 (0..7)
    int bonus = 0;
    while (pawns) {
        int sq = std::countr_zero(pawns);
        pawns &= (pawns - 1);
        int rank = sq / 8;
        // White pawns starten auf rank 1 und wollen hoch:
        // rank 2..6 -> kleine Boni, rank 6 (7. Reihe) -> deutlicher Bonus
        static constexpr int BY_RANK[8] = {0, 0, 2, 4, 6, 10, 20, 0};
        bonus += BY_RANK[rank];
    }
    return bonus;
}

static int pawnAdvanceBonusBlack(uint64_t pawns) {
    // Black pawns wollen runter; wir spiegeln über mirrorSquare-Logik
    int bonus = 0;
    while (pawns) {
        int sq = std::countr_zero(pawns);
        pawns &= (pawns - 1);
        int msq = mirrorSquare(sq);
        int rank = msq / 8; // jetzt wie "White"
        static constexpr int BY_RANK[8] = {0, 0, 2, 4, 6, 10, 20, 0};
        bonus += BY_RANK[rank];
    }
    return bonus;
}

static int kingPawnShieldPenalty(const Board& b, Color side) {
    const int ksq = b.kingSq[side];
    if (ksq < 0) return 0;

    const int kFile = ksq % 8;
    const int kRank = ksq / 8;

    const uint64_t pawns = b.bitboards[side][PAWN];

    // Wir prüfen 1–2 Reihen "vor" dem König in den Files (kFile-1..kFile+1)
    // White: vorwärts = +1/+2 Rank; Black: -1/-2 Rank.
    int penalty = 0;

    for (int df = -1; df <= 1; ++df) {
        int f = kFile + df;
        if (f < 0 || f > 7) continue;

        auto hasPawnAt = [&](int r) -> bool {
            if (r < 0 || r > 7) return false;
            int sq = r * 8 + f;
            return (pawns & sqBB(sq)) != 0ULL;
        };

        if (side == WHITE) {
            bool shield1 = hasPawnAt(kRank + 1);
            bool shield2 = hasPawnAt(kRank + 2);
            if (!shield1) penalty += 6;
            if (!shield2) penalty += 3;
        } else {
            bool shield1 = hasPawnAt(kRank - 1);
            bool shield2 = hasPawnAt(kRank - 2);
            if (!shield1) penalty += 6;
            if (!shield2) penalty += 3;
        }
    }

    return penalty;
}

int evaluate(const Board& board) {
    // Materialwerte

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
        constexpr int V[6] = {100, 320, 330, 500, 900, 0};
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

    // --- Neu: Pawn-Advance (Promotion-Drang) ---
    score += pawnAdvanceBonusWhite(board.bitboards[WHITE][PAWN]);
    score -= pawnAdvanceBonusBlack(board.bitboards[BLACK][PAWN]);

    // --- Neu: King Safety (Pawn Shield) ---
    // Fehlender Shield ist schlecht für die jeweilige Seite:
    score -= kingPawnShieldPenalty(board, WHITE);
    score += kingPawnShieldPenalty(board, BLACK);

    return score;
}

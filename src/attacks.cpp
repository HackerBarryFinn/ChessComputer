#include "../include/attacks.h"
#include <cstdint>

static inline uint64_t sqBB(int sq) { return 1ULL << sq; }
static inline bool inBoard(int r, int f) { return r >= 0 && r < 8 && f >= 0 && f < 8; }

int findKingSquare(const Board& board, Color side) {
    uint64_t k = board.bitboards[side][KING];
    if (k == 0ULL) return -1;

    // Genau ein König: linearer Scan ist ok
    for (int sq = 0; sq < 64; ++sq) {
        if (k & sqBB(sq)) return sq;
    }
    return -1;
}

static uint64_t pawnAttackMask(Color byColor, uint64_t pawns) {
    constexpr uint64_t FILE_A = 0x0101010101010101ULL;
    constexpr uint64_t FILE_H = 0x8080808080808080ULL;

    if (byColor == WHITE) {
        // White pawns attack upwards (+7, +9)
        uint64_t left  = (pawns & ~FILE_A) << 7;
        uint64_t right = (pawns & ~FILE_H) << 9;
        return left | right;
    } else {
        // Black pawns attack downwards (-7, -9)
        uint64_t left  = (pawns & ~FILE_H) >> 7;
        uint64_t right = (pawns & ~FILE_A) >> 9;
        return left | right;
    }
}

static uint64_t knightAttackMask(int sq) {
    constexpr uint64_t FILE_A = 0x0101010101010101ULL;
    constexpr uint64_t FILE_B = 0x0202020202020202ULL;
    constexpr uint64_t FILE_G = 0x4040404040404040ULL;
    constexpr uint64_t FILE_H = 0x8080808080808080ULL;

    const uint64_t NOT_FILE_A  = ~FILE_A;
    const uint64_t NOT_FILE_H  = ~FILE_H;
    const uint64_t NOT_FILE_AB = ~(FILE_A | FILE_B);
    const uint64_t NOT_FILE_GH = ~(FILE_G | FILE_H);

    uint64_t b = sqBB(sq);
    uint64_t attacks = 0ULL;

    attacks |= (b << 17) & NOT_FILE_A;
    attacks |= (b << 15) & NOT_FILE_H;
    attacks |= (b << 10) & NOT_FILE_AB;
    attacks |= (b << 6)  & NOT_FILE_GH;

    attacks |= (b >> 17) & NOT_FILE_H;
    attacks |= (b >> 15) & NOT_FILE_A;
    attacks |= (b >> 10) & NOT_FILE_GH;
    attacks |= (b >> 6)  & NOT_FILE_AB;

    return attacks;
}

static uint64_t kingAttackMask(int sq) {
    uint64_t attacks = 0ULL;
    int r = sq / 8;
    int f = sq % 8;

    for (int dr = -1; dr <= 1; ++dr) {
        for (int df = -1; df <= 1; ++df) {
            if (dr == 0 && df == 0) continue;
            int rr = r + dr;
            int ff = f + df;
            if (!inBoard(rr, ff)) continue;
            attacks |= sqBB(rr * 8 + ff);
        }
    }
    return attacks;
}

static bool rayAttackedBy(const Board& board, int square, Color byColor, int dr, int df,
                          bool bishopLike, bool rookLike) {
    int r = square / 8;
    int f = square % 8;

    while (true) {
        r += dr;
        f += df;
        if (!inBoard(r, f)) return false;

        int sq = r * 8 + f;
        uint64_t bb = sqBB(sq);

        if ((board.allOccupied & bb) == 0ULL) {
            continue;
        }

        // Erstes Piece auf dem Ray entscheidet
        if (board.occupied[byColor] & bb) {
            if (bishopLike) {
                if ((board.bitboards[byColor][BISHOP] & bb) || (board.bitboards[byColor][QUEEN] & bb))
                    return true;
            }
            if (rookLike) {
                if ((board.bitboards[byColor][ROOK] & bb) || (board.bitboards[byColor][QUEEN] & bb))
                    return true;
            }
        }
        return false; // blockiert durch irgendein Piece
    }
}

bool isSquareAttacked(const Board& board, int square, Color byColor) {
    if (square < 0 || square >= 64) return false;

    const uint64_t target = sqBB(square);

    // Pawns
    if (pawnAttackMask(byColor, board.bitboards[byColor][PAWN]) & target) return true;

    // Knights
    if (knightAttackMask(square) & board.bitboards[byColor][KNIGHT]) return true;

    // King adjacency
    int ksq = findKingSquare(board, byColor);
    if (ksq != -1 && (kingAttackMask(ksq) & target)) return true;

    // Diagonalen (Bishop/Queen)
    if (rayAttackedBy(board, square, byColor, +1, +1, true, false)) return true;
    if (rayAttackedBy(board, square, byColor, +1, -1, true, false)) return true;
    if (rayAttackedBy(board, square, byColor, -1, +1, true, false)) return true;
    if (rayAttackedBy(board, square, byColor, -1, -1, true, false)) return true;

    // Geraden (Rook/Queen)
    if (rayAttackedBy(board, square, byColor, +1, 0, false, true)) return true;
    if (rayAttackedBy(board, square, byColor, -1, 0, false, true)) return true;
    if (rayAttackedBy(board, square, byColor, 0, +1, false, true)) return true;
    if (rayAttackedBy(board, square, byColor, 0, -1, false, true)) return true;

    return false;
}
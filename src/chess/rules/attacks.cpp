#include "chess/rules/attacks.h"
#include <cstdint>

/**
 * Creates a bitboard with a single bit set for the specified square.
 *
 * @param sq An integer representing the square index (0 to 63).
 * @return A 64-bit integer (bitboard) where only the bit corresponding to
 *         the specified square index is set. All other bits are zero.
 */
static uint64_t sqBB(int sq) { return 1ULL << sq; }

/**
 * Checks if the given rank and file coordinates are within the bounds of the chessboard.
 *
 * @param r An integer representing the rank (row) of the chessboard (0 to 7).
 * @param f An integer representing the file (column) of the chessboard (0 to 7).
 * @return A boolean value: true if the given rank and file are within the bounds
 *         of the chessboard, false otherwise.
 */
static bool inBoard(int r, int f) { return r >= 0 && r < 8 && f >= 0 && f < 8; }

/**
 * Finds the square index of the king for the specified side on the given board.
 *
 * @param board The current chess board state.
 * @param side The color of the side (WHITE or BLACK) whose king's position
 *             is being queried.
 * @return An integer representing the square index of the king's position on
 *         the board. Returns -1 if the king is not present.
 */
int findKingSquare(const Board& board, Color side) {
    return board.kingSq[side];
}

/**
 * Generates a bitboard representing all squares that can be attacked by pawns
 * of a given color from their current positions.
 *
 * @param byColor The color of the pawns (WHITE or BLACK).
 * @param pawns A 64-bit bitboard where each bit set to 1 represents the position
 *              of a pawn on the board for the given color.
 * @return A 64-bit bitboard where each bit set to 1 represents a square that can
 *         be attacked by the pawns of the specified color.
 */
static uint64_t pawnAttackMask(Color byColor, uint64_t pawns) {
    constexpr uint64_t FILE_A = 0x0101010101010101ULL;
    constexpr uint64_t FILE_H = 0x8080808080808080ULL;

    if (byColor == WHITE) {
        uint64_t left  = (pawns & ~FILE_A) << 7;
        uint64_t right = (pawns & ~FILE_H) << 9;
        return left | right;
    } else {
        uint64_t left  = (pawns & ~FILE_H) >> 7;
        uint64_t right = (pawns & ~FILE_A) >> 9;
        return left | right;
    }
}

/**
 * Computes the attack mask for a knight from a given square on a chessboard.
 *
 * @param sq An integer representing the square index (0 to 63) where the
 *           knight is located.
 * @return A 64-bit integer (bitboard) where the bits corresponding to the
 *         squares the knight can attack are set to 1. All other bits are 0.
 */
static uint64_t knightAttackMask(int sq) {
    constexpr uint64_t FILE_A = 0x0101010101010101ULL;
    constexpr uint64_t FILE_B = 0x0202020202020202ULL;
    constexpr uint64_t FILE_G = 0x4040404040404040ULL;
    constexpr uint64_t FILE_H = 0x8080808080808080ULL;

    constexpr uint64_t NOT_FILE_A  = ~FILE_A;
    constexpr uint64_t NOT_FILE_H  = ~FILE_H;
    constexpr uint64_t NOT_FILE_AB = ~(FILE_A | FILE_B);
    constexpr uint64_t NOT_FILE_GH = ~(FILE_G | FILE_H);

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

/**
 * Determines whether a square is attacked along a specified ray direction
 * by a piece of a given color.
 *
 * @param board The current state of the chessboard.
 * @param square An integer representing the square index (0 to 63) being checked.
 * @param byColor The color of the attacking pieces (WHITE or BLACK).
 * @param dr The change in the rank (row) direction for the ray.
 * @param df The change in the file (column) direction for the ray.
 * @param bishopLike A boolean indicating whether to consider bishop-like pieces (BISHOP or QUEEN).
 * @param rookLike A boolean indicating whether to consider rook-like pieces (ROOK or QUEEN).
 * @return True if the square is attacked along the specified ray by the given color and piece type(s);
 *         false otherwise.
 */
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
        return false;
    }
}

/**
 * Checks if a specific square on the chessboard is under attack by a given color.
 *
 * @param board A reference to the chessboard, which contains the positions of all pieces.
 * @param square An integer representing the square index (0 to 63) to check for an attack.
 * @param byColor The color of the pieces to check for the attacking threat (WHITE or BLACK).
 * @return true if the specified square is attacked by a piece of the given color; false otherwise.
 */
bool isSquareAttacked(const Board& board, int square, Color byColor) {
    if (square < 0 || square >= 64) return false;

    const uint64_t target = sqBB(square);

    if (pawnAttackMask(byColor, board.bitboards[byColor][PAWN]) & target) return true;
    if (knightAttackMask(square) & board.bitboards[byColor][KNIGHT]) return true;

    const int ksq = board.kingSq[byColor];
    if (ksq != -1 && (kingAttackMask(ksq) & target)) return true;

    if (rayAttackedBy(board, square, byColor, +1, +1, true, false)) return true;
    if (rayAttackedBy(board, square, byColor, +1, -1, true, false)) return true;
    if (rayAttackedBy(board, square, byColor, -1, +1, true, false)) return true;
    if (rayAttackedBy(board, square, byColor, -1, -1, true, false)) return true;

    if (rayAttackedBy(board, square, byColor, +1, 0, false, true)) return true;
    if (rayAttackedBy(board, square, byColor, -1, 0, false, true)) return true;
    if (rayAttackedBy(board, square, byColor, 0, +1, false, true)) return true;
    if (rayAttackedBy(board, square, byColor, 0, -1, false, true)) return true;

    return false;
}
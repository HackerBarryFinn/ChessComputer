#include "chess/pieces/sliding.h"

/**
 * Generates a bitboard with a single bit set corresponding to the given square index.
 *
 * @param sq The square index (0 to 63, where 0 corresponds to a1 and 63 corresponds to h8).
 * @return A 64-bit unsigned integer with a single bit set at the bit position corresponding to the given square.
 */
static inline uint64_t sqBB(int sq) { return 1ULL << sq; }

/**
 * Generates all valid moves in a specific sliding direction for a sliding piece (e.g., rook, bishop) on the chessboard.
 * Only considers moves along a ray in a single direction (combination of rank and file deltas).
 *
 * @param out The list where valid generated moves will be stored.
 * @param board The current state of the chessboard, including piece positions.
 * @param side The color of the moving side (WHITE or BLACK).
 * @param movedPiece The type of the sliding piece generating moves (e.g., BISHOP, ROOK).
 * @param from The starting square index (0 to 63) of the sliding piece.
 * @param dr The change in rank (row) for each step along the ray.
 * @param df The change in file (column) for each step along the ray.
 */
static void addRayMoves(MoveList& out,
                        const Board& board,
                        Color side,
                        PieceType movedPiece,
                        int from,
                        int dr,
                        int df) {
    const uint64_t own = board.occupied[side];
    const uint64_t enemy = (side == WHITE) ? board.occupied[BLACK] : board.occupied[WHITE];

    int r = from / 8;
    int f = from % 8;

    while (true) {
        r += dr;
        f += df;
        if (r < 0 || r > 7 || f < 0 || f > 7) break;

        int to = r * 8 + f;
        uint64_t toBB = sqBB(to);

        if (own & toBB) break;

        Move m;
        m.from = from;
        m.to = to;
        m.moved = movedPiece;

        if (enemy & toBB) {
            m.flags = CAPTURE;
            out.push(m);
            break;
        } else {
            m.flags = QUIET;
            out.push(m);
        }
    }
}

void generateBishopMoves(const Board& board, Color side, MoveList& out) {
    uint64_t bb = board.bitboards[side][BISHOP];
    while (bb) {
        int from = 0;
        while (((bb >> from) & 1ULL) == 0ULL) ++from;
        bb &= (bb - 1);

        addRayMoves(out, board, side, BISHOP, from, +1, +1);
        addRayMoves(out, board, side, BISHOP, from, +1, -1);
        addRayMoves(out, board, side, BISHOP, from, -1, +1);
        addRayMoves(out, board, side, BISHOP, from, -1, -1);
    }
}

void generateRookMoves(const Board& board, Color side, MoveList& out) {
    uint64_t bb = board.bitboards[side][ROOK];
    while (bb) {
        int from = 0;
        while (((bb >> from) & 1ULL) == 0ULL) ++from;
        bb &= (bb - 1);

        addRayMoves(out, board, side, ROOK, from, +1, 0);
        addRayMoves(out, board, side, ROOK, from, -1, 0);
        addRayMoves(out, board, side, ROOK, from, 0, +1);
        addRayMoves(out, board, side, ROOK, from, 0, -1);
    }
}

void generateQueenMoves(const Board& board, Color side, MoveList& out) {
    uint64_t bb = board.bitboards[side][QUEEN];
    while (bb) {
        int from = 0;
        while (((bb >> from) & 1ULL) == 0ULL) ++from;
        bb &= (bb - 1);

        addRayMoves(out, board, side, QUEEN, from, +1, 0);
        addRayMoves(out, board, side, QUEEN, from, -1, 0);
        addRayMoves(out, board, side, QUEEN, from, 0, +1);
        addRayMoves(out, board, side, QUEEN, from, 0, -1);

        addRayMoves(out, board, side, QUEEN, from, +1, +1);
        addRayMoves(out, board, side, QUEEN, from, +1, -1);
        addRayMoves(out, board, side, QUEEN, from, -1, +1);
        addRayMoves(out, board, side, QUEEN, from, -1, -1);
    }
}

/**
 * Generates a list of legal bishop moves for the given side on the provided board.
 *
 * This function internally leverages a temporary `MoveList` to collect all legal
 * bishop moves for the specified side, spanning all possible diagonal directions.
 * The resulting moves are returned as a `std::vector` of `Move` instances, which
 * encapsulate the details of each individual bishop move.
 *
 * @param board The game board representing the current position of all pieces.
 * @param side The color (side) for which bishop moves are to be generated.
 * @return A `std::vector` containing all legal moves for bishops of the given side.
 */
std::vector<Move> generateBishopMoves(const Board& board, Color side) {
    MoveList tmp;
    generateBishopMoves(board, side, tmp);
    return std::vector<Move>(tmp.data.begin(), tmp.data.begin() + tmp.size);
}

/**
 * Generates all possible legal rook moves for a given board and side.
 *
 * Internally, this method computes rook moves for the specified side on the current
 * board configuration using sliding piece mechanics.
 *
 * @param board The chess board state, including all pieces and their positions.
 * @param side The side for which rook moves will be generated (WHITE or BLACK).
 * @return A vector containing all generated rook moves for the specified side.
 */
std::vector<Move> generateRookMoves(const Board& board, Color side) {
    MoveList tmp;
    generateRookMoves(board, side, tmp);
    return std::vector<Move>(tmp.data.begin(), tmp.data.begin() + tmp.size);
}

/**
 * Generates all possible legal queen moves for the specified side on the given board.
 *
 * @param board The chessboard representation, including piece positions and game state.
 * @param side The side for which queen moves are to be generated (WHITE or BLACK).
 * @return A vector containing all legal queen moves for the specified side.
 */
std::vector<Move> generateQueenMoves(const Board& board, Color side) {
    MoveList tmp;
    generateQueenMoves(board, side, tmp);
    return std::vector<Move>(tmp.data.begin(), tmp.data.begin() + tmp.size);
}
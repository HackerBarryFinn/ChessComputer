#include "../../include/figures/sliding.h"

static inline uint64_t sqBB(int sq) { return 1ULL << sq; }

static void addRayMoves(std::vector<Move>& moves,
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

        if (own & toBB) {
            // eigene Figur blockt: Ray endet
            break;
        }

        Move m;
        m.from = from;
        m.to = to;
        m.moved = movedPiece;

        if (enemy & toBB) {
            m.flags = CAPTURE;
            moves.push_back(m);
            // nach Capture endet der Ray
            break;
        } else {
            m.flags = QUIET;
            moves.push_back(m);
        }
    }
}

static std::vector<int> squaresFromBitboard(uint64_t bb) {
    std::vector<int> squares;
    while (bb) {
        // portable LSB-scan ohne utils.h (wir haben wenige Pieces)
        int sq = 0;
        while (((bb >> sq) & 1ULL) == 0ULL) ++sq;
        squares.push_back(sq);
        bb &= (bb - 1);
    }
    return squares;
}

std::vector<Move> generateBishopMoves(const Board& board, Color side) {
    std::vector<Move> moves;
    auto bishops = squaresFromBitboard(board.bitboards[side][BISHOP]);

    for (int from : bishops) {
        addRayMoves(moves, board, side, BISHOP, from, +1, +1); // NE
        addRayMoves(moves, board, side, BISHOP, from, +1, -1); // NW
        addRayMoves(moves, board, side, BISHOP, from, -1, +1); // SE
        addRayMoves(moves, board, side, BISHOP, from, -1, -1); // SW
    }

    return moves;
}

std::vector<Move> generateRookMoves(const Board& board, Color side) {
    std::vector<Move> moves;
    auto rooks = squaresFromBitboard(board.bitboards[side][ROOK]);

    for (int from : rooks) {
        addRayMoves(moves, board, side, ROOK, from, +1, 0);  // N
        addRayMoves(moves, board, side, ROOK, from, -1, 0);  // S
        addRayMoves(moves, board, side, ROOK, from, 0, +1);  // E
        addRayMoves(moves, board, side, ROOK, from, 0, -1);  // W
    }

    return moves;
}

std::vector<Move> generateQueenMoves(const Board& board, Color side) {
    std::vector<Move> moves;
    auto queens = squaresFromBitboard(board.bitboards[side][QUEEN]);

    for (int from : queens) {
        // Rook-like
        addRayMoves(moves, board, side, QUEEN, from, +1, 0);
        addRayMoves(moves, board, side, QUEEN, from, -1, 0);
        addRayMoves(moves, board, side, QUEEN, from, 0, +1);
        addRayMoves(moves, board, side, QUEEN, from, 0, -1);

        // Bishop-like
        addRayMoves(moves, board, side, QUEEN, from, +1, +1);
        addRayMoves(moves, board, side, QUEEN, from, +1, -1);
        addRayMoves(moves, board, side, QUEEN, from, -1, +1);
        addRayMoves(moves, board, side, QUEEN, from, -1, -1);
    }

    return moves;
}
#include "../../include/figures/sliding.h"

static inline uint64_t sqBB(int sq) { return 1ULL << sq; }

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

std::vector<Move> generateBishopMoves(const Board& board, Color side) {
    MoveList tmp;
    generateBishopMoves(board, side, tmp);
    return std::vector<Move>(tmp.data.begin(), tmp.data.begin() + tmp.size);
}

std::vector<Move> generateRookMoves(const Board& board, Color side) {
    MoveList tmp;
    generateRookMoves(board, side, tmp);
    return std::vector<Move>(tmp.data.begin(), tmp.data.begin() + tmp.size);
}

std::vector<Move> generateQueenMoves(const Board& board, Color side) {
    MoveList tmp;
    generateQueenMoves(board, side, tmp);
    return std::vector<Move>(tmp.data.begin(), tmp.data.begin() + tmp.size);
}
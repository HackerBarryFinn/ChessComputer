#include "chess/pieces/king.h"

static inline uint64_t sqBB(int sq) { return 1ULL << sq; }

void generateKingMoves(const Board& board, Color side, MoveList& out) {
    uint64_t kingBB = board.bitboards[side][KING];
    if (kingBB == 0ULL) return;

    int from = 0;
    while (((kingBB >> from) & 1ULL) == 0ULL) ++from;

    uint64_t own = board.occupied[side];
    uint64_t enemy = (side == WHITE) ? board.occupied[BLACK] : board.occupied[WHITE];

    int fromFile = from % 8;
    int fromRank = from / 8;

    for (int dr = -1; dr <= 1; ++dr) {
        for (int df = -1; df <= 1; ++df) {
            if (dr == 0 && df == 0) continue;

            int r = fromRank + dr;
            int f = fromFile + df;
            if (r < 0 || r > 7 || f < 0 || f > 7) continue;

            int to = r * 8 + f;
            uint64_t toBB = sqBB(to);

            if (own & toBB) continue;

            Move m;
            m.from = from;
            m.to = to;
            m.moved = KING;
            m.flags = (enemy & toBB) ? CAPTURE : QUIET;
            out.push(m);
        }
    }

    if (side == WHITE) {
        if (from == 4) {
            if (board.whiteKingsideCastle) {
                bool squaresEmpty = ((board.allOccupied & (sqBB(5) | sqBB(6))) == 0ULL);
                bool rookPresent = (board.bitboards[WHITE][ROOK] & sqBB(7)) != 0ULL;
                if (squaresEmpty && rookPresent) {
                    Move m;
                    m.from = 4;
                    m.to = 6;
                    m.moved = KING;
                    m.flags = CASTLING;
                    out.push(m);
                }
            }
            if (board.whiteQueensideCastle) {
                bool squaresEmpty = ((board.allOccupied & (sqBB(1) | sqBB(2) | sqBB(3))) == 0ULL);
                bool rookPresent = (board.bitboards[WHITE][ROOK] & sqBB(0)) != 0ULL;
                if (squaresEmpty && rookPresent) {
                    Move m;
                    m.from = 4;
                    m.to = 2;
                    m.moved = KING;
                    m.flags = CASTLING;
                    out.push(m);
                }
            }
        }
    } else {
        if (from == 60) {
            if (board.blackKingsideCastle) {
                bool squaresEmpty = ((board.allOccupied & (sqBB(61) | sqBB(62))) == 0ULL);
                bool rookPresent = (board.bitboards[BLACK][ROOK] & sqBB(63)) != 0ULL;
                if (squaresEmpty && rookPresent) {
                    Move m;
                    m.from = 60;
                    m.to = 62;
                    m.moved = KING;
                    m.flags = CASTLING;
                    out.push(m);
                }
            }
            if (board.blackQueensideCastle) {
                bool squaresEmpty = ((board.allOccupied & (sqBB(57) | sqBB(58) | sqBB(59))) == 0ULL);
                bool rookPresent = (board.bitboards[BLACK][ROOK] & sqBB(56)) != 0ULL;
                if (squaresEmpty && rookPresent) {
                    Move m;
                    m.from = 60;
                    m.to = 58;
                    m.moved = KING;
                    m.flags = CASTLING;
                    out.push(m);
                }
            }
        }
    }
}

std::vector<Move> generateKingMoves(const Board& board, Color side) {
    MoveList tmp;
    generateKingMoves(board, side, tmp);
    return std::vector<Move>(tmp.data.begin(), tmp.data.begin() + tmp.size);
}
#include "chess/figures/knights.h"
#include "../../../include/app/utils.h"

static inline uint64_t sqBB(int sq) { return 1ULL << sq; }

void generateKnightMoves(const Board& board, Color side, MoveList& out) {
    uint64_t knights = board.bitboards[side][KNIGHT];
    uint64_t own = board.occupied[side];
    uint64_t enemy = (side == WHITE) ? board.occupied[BLACK] : board.occupied[WHITE];

    constexpr uint64_t FILE_A = 0x0101010101010101ULL;
    constexpr uint64_t FILE_B = 0x0202020202020202ULL;
    constexpr uint64_t FILE_G = 0x4040404040404040ULL;
    constexpr uint64_t FILE_H = 0x8080808080808080ULL;

    constexpr uint64_t NOT_FILE_A  = ~FILE_A;
    constexpr uint64_t NOT_FILE_H  = ~FILE_H;
    constexpr uint64_t NOT_FILE_AB = ~(FILE_A | FILE_B);
    constexpr uint64_t NOT_FILE_GH = ~(FILE_G | FILE_H);

    while (knights) {
        int from = bitScanForward(knights);
        knights &= knights - 1;

        uint64_t fromBB = sqBB(from);

        uint64_t attacks = 0ULL;
        attacks |= (fromBB << 17) & NOT_FILE_A;
        attacks |= (fromBB << 15) & NOT_FILE_H;
        attacks |= (fromBB << 10) & NOT_FILE_AB;
        attacks |= (fromBB << 6)  & NOT_FILE_GH;

        attacks |= (fromBB >> 17) & NOT_FILE_H;
        attacks |= (fromBB >> 15) & NOT_FILE_A;
        attacks |= (fromBB >> 10) & NOT_FILE_GH;
        attacks |= (fromBB >> 6)  & NOT_FILE_AB;

        attacks &= ~own;

        uint64_t quiet = attacks & ~enemy;
        uint64_t t = quiet;
        while (t) {
            int to = bitScanForward(t);
            t &= t - 1;

            Move m;
            m.from = from;
            m.to = to;
            m.moved = KNIGHT;
            m.flags = QUIET;
            out.push(m);
        }

        uint64_t caps = attacks & enemy;
        t = caps;
        while (t) {
            int to = bitScanForward(t);
            t &= t - 1;

            Move m;
            m.from = from;
            m.to = to;
            m.moved = KNIGHT;
            m.flags = CAPTURE;
            out.push(m);
        }
    }
}

std::vector<Move> generateKnightMoves(const Board& board, Color side) {
    MoveList tmp;
    generateKnightMoves(board, side, tmp);
    return std::vector<Move>(tmp.data.begin(), tmp.data.begin() + tmp.size);
}
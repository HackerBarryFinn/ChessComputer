#include "../../include/figures/king.h"

static inline uint64_t sqBB(int sq) { return 1ULL << sq; }

std::vector<Move> generateKingMoves(const Board& board, Color side) {
    std::vector<Move> moves;

    uint64_t kingBB = board.bitboards[side][KING];
    if (kingBB == 0ULL) return moves; // kein König auf dem Board (z.B. Teststellungen)

    // In normalen Stellungen gibt es genau einen König. Wir nehmen das erste Bit.
    int from = 0;
#ifdef _MSC_VER
    // wir könnten bitScanForward nutzen, aber um Abhängigkeiten klein zu halten:
    // einfacher linear (ist nur 1 King) oder du bindest utils.h ein.
    while (((kingBB >> from) & 1ULL) == 0ULL) ++from;
#else
    while (((kingBB >> from) & 1ULL) == 0ULL) ++from;
#endif

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

            if (own & toBB) continue; // eigenes Piece blockt

            Move m;
            m.from = from;
            m.to = to;
            m.moved = KING;

            if (enemy & toBB) {
                m.flags = CAPTURE;
            } else {
                m.flags = QUIET;
            }

            moves.push_back(m);
        }
    }

    return moves;
}
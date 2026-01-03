#include "../../include/figures/king.h"

static inline uint64_t sqBB(int sq) { return 1ULL << sq; }

std::vector<Move> generateKingMoves(const Board& board, Color side) {
    std::vector<Move> moves;

    uint64_t kingBB = board.bitboards[side][KING];
    if (kingBB == 0ULL) return moves;

    int from = 0;
    while (((kingBB >> from) & 1ULL) == 0ULL) ++from;

    uint64_t own = board.occupied[side];
    uint64_t enemy = (side == WHITE) ? board.occupied[BLACK] : board.occupied[WHITE];

    int fromFile = from % 8;
    int fromRank = from / 8;

    // Normale King-Moves (8 Nachbarn)
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
            moves.push_back(m);
        }
    }

    // Rochade (pseudo-legal: Rechte + Leerfelder + Turm vorhanden)
    // Attack-Checks (nicht im/über Schach) machen wir im Legal-Filter.
    if (side == WHITE) {
        // King muss auf e1 stehen
        if (from == 4) {
            // White kingside: e1->g1, rook h1->f1
            if (board.whiteKingsideCastle) {
                bool squaresEmpty = ((board.allOccupied & (sqBB(5) | sqBB(6))) == 0ULL);
                bool rookPresent = (board.bitboards[WHITE][ROOK] & sqBB(7)) != 0ULL;
                if (squaresEmpty && rookPresent) {
                    Move m;
                    m.from = 4;
                    m.to = 6;
                    m.moved = KING;
                    m.flags = CASTLING;
                    moves.push_back(m);
                }
            }

            // White queenside: e1->c1, rook a1->d1
            if (board.whiteQueensideCastle) {
                bool squaresEmpty = ((board.allOccupied & (sqBB(1) | sqBB(2) | sqBB(3))) == 0ULL);
                bool rookPresent = (board.bitboards[WHITE][ROOK] & sqBB(0)) != 0ULL;
                if (squaresEmpty && rookPresent) {
                    Move m;
                    m.from = 4;
                    m.to = 2;
                    m.moved = KING;
                    m.flags = CASTLING;
                    moves.push_back(m);
                }
            }
        }
    } else {
        // Black king on e8
        if (from == 60) {
            // Black kingside: e8->g8, rook h8->f8
            if (board.blackKingsideCastle) {
                bool squaresEmpty = ((board.allOccupied & (sqBB(61) | sqBB(62))) == 0ULL);
                bool rookPresent = (board.bitboards[BLACK][ROOK] & sqBB(63)) != 0ULL;
                if (squaresEmpty && rookPresent) {
                    Move m;
                    m.from = 60;
                    m.to = 62;
                    m.moved = KING;
                    m.flags = CASTLING;
                    moves.push_back(m);
                }
            }

            // Black queenside: e8->c8, rook a8->d8
            if (board.blackQueensideCastle) {
                bool squaresEmpty = ((board.allOccupied & (sqBB(57) | sqBB(58) | sqBB(59))) == 0ULL);
                bool rookPresent = (board.bitboards[BLACK][ROOK] & sqBB(56)) != 0ULL;
                if (squaresEmpty && rookPresent) {
                    Move m;
                    m.from = 60;
                    m.to = 58;
                    m.moved = KING;
                    m.flags = CASTLING;
                    moves.push_back(m);
                }
            }
        }
    }

    return moves;
}
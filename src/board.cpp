#include "board.h"

void initBitboards(Board& board) {
    board.bitboards[WHITE][PAWN]   = 0x000000000000FF00ULL;
    board.bitboards[WHITE][ROOK]   = 0x0000000000000081ULL;
    board.bitboards[WHITE][KNIGHT] = 0x0000000000000042ULL;
    board.bitboards[WHITE][BISHOP] = 0x0000000000000024ULL;
    board.bitboards[WHITE][QUEEN]  = 0x0000000000000008ULL;
    board.bitboards[WHITE][KING]   = 0x0000000000000010ULL;

    board.bitboards[BLACK][PAWN]   = 0x00FF000000000000ULL;
    board.bitboards[BLACK][ROOK]   = 0x8100000000000000ULL;
    board.bitboards[BLACK][KNIGHT] = 0x4200000000000000ULL;
    board.bitboards[BLACK][BISHOP] = 0x2400000000000000ULL;
    board.bitboards[BLACK][QUEEN]  = 0x0800000000000000ULL;
    board.bitboards[BLACK][KING]   = 0x1000000000000000ULL;

    board.occupied[WHITE] = 0;
    board.occupied[BLACK] = 0;

    for (int pt = PAWN; pt <= KING; ++pt) {
        board.occupied[WHITE] |= board.bitboards[WHITE][pt];
        board.occupied[BLACK] |= board.bitboards[BLACK][pt];
    }

    board.allOccupied = board.occupied[WHITE] | board.occupied[BLACK];
}

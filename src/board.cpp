#include "board.h"
#include <iostream>

void initBitboards(Board &board) {
    board.bitboards[WHITE][PAWN] = 0x000000000000FF00ULL;
    board.bitboards[WHITE][ROOK] = 0x0000000000000081ULL;
    board.bitboards[WHITE][KNIGHT] = 0x0000000000000042ULL;
    board.bitboards[WHITE][BISHOP] = 0x0000000000000024ULL;
    board.bitboards[WHITE][QUEEN] = 0x0000000000000008ULL;
    board.bitboards[WHITE][KING] = 0x0000000000000010ULL;

    board.bitboards[BLACK][PAWN] = 0x00FF000000000000ULL;
    board.bitboards[BLACK][ROOK] = 0x8100000000000000ULL;
    board.bitboards[BLACK][KNIGHT] = 0x4200000000000000ULL;
    board.bitboards[BLACK][BISHOP] = 0x2400000000000000ULL;
    board.bitboards[BLACK][QUEEN] = 0x0800000000000000ULL;
    board.bitboards[BLACK][KING] = 0x1000000000000000ULL;

    board.occupied[WHITE] = 0;
    board.occupied[BLACK] = 0;

    for (int pt = PAWN; pt <= KING; ++pt) {
        board.occupied[WHITE] |= board.bitboards[WHITE][pt];
        board.occupied[BLACK] |= board.bitboards[BLACK][pt];
    }

    board.allOccupied = board.occupied[WHITE] | board.occupied[BLACK];
}

void printBitboard(const uint64_t bitboard) {
    for (int rank = 7; rank >= 0; --rank) {
        std::cout << rank + 1 << "  ";
        for (int file = 0; file < 8; ++file) {
            const int square = rank * 8 + file;
            std::cout << ((bitboard >> square) & 1ULL ? "1 " : ". ");
        }
        std::cout << std::endl;
    }
    std::cout << "\n   a b c d e f g h\n" << std::endl;
}

void printBoard(const Board &board) {
    for (int rank = 7; rank >= 0; --rank) {
        std::cout << rank + 1 << "  ";
        for (int file = 0; file < 8; ++file) {
            int square = rank * 8 + file;
            char pieceChar = '.';

            for (int color = WHITE; color <= BLACK; ++color) {
                for (int pt = PAWN; pt <= KING; ++pt) {
                    if ((board.bitboards[color][pt] >> square) & 1ULL) {
                        static const char symbols[2][6] = {
                            {'P', 'N', 'B', 'R', 'Q', 'K'},
                            {'p', 'n', 'b', 'r', 'q', 'k'}
                        };
                        pieceChar = symbols[color][pt];
                    }
                }
            }
            std::cout << pieceChar << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "\n   a b c d e f g h\n" << std::endl;
}

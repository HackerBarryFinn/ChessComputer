#include <iostream>
#include "board.h"

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

int main() {
    Board board;
    initBitboards(board);

    std::cout << "Startposition initialisiert!" << std::endl;

    printBitboard(board.bitboards[WHITE][ROOK]);

    return 0;
}

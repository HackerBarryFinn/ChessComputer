#include <iostream>
#include "board.h"

int main() {
    Board board;
    initBitboards(board);

    std::cout << "Startposition initialisiert!" << std::endl;

    printBitboard(board.bitboards[WHITE][ROOK]);

    return 0;
}

#include <iostream>
#include "board.h"
#include "movegen.h"

int main() {
    Board board;
    initBitboards(board);

    std::cout << "Startposition initialisiert!" << std::endl;

    // printBitboard(board.bitboards[WHITE][PAWN]);
    // printBoard(board);

    auto pawnMoves = generatePawnMoves(board, WHITE);

    std::cout << "Weiße Bauernzüge: " << pawnMoves.size() << std::endl;


    return 0;
}

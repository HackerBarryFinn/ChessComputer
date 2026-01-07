#include "../include/perft.h"
#include "../include/movegen.h"
#include "../include/makemove.h"

uint64_t perft(Board& board, int depth) {
    if (depth <= 0) return 1ULL;

    uint64_t nodes = 0ULL;
    Color side = board.sideToMove;

    auto moves = generateLegalMoves(board, side);
    for (const auto& m : moves) {
        UndoState u{};
        if (!makeMove(board, m, u)) continue;

        nodes += perft(board, depth - 1);

        unmakeMove(board, m, u);
    }

    return nodes;
}
#include "../include/perft.h"
#include "../include/movegen.h"
#include "../include/makemove.h"

uint64_t perft(Board& board, int depth, MoveList* moveStack, int ply) {
    if (depth <= 0) return 1ULL;

    uint64_t nodes = 0ULL;
    const Color side = board.sideToMove;

    MoveList& moves = moveStack[ply];
    generateLegalMoves(board, side, moves);

    for (int i = 0; i < moves.size; ++i) {
        const Move& m = moves[i];

        UndoState u{};
        if (!makeMove(board, m, u)) continue;

        nodes += perft(board, depth - 1, moveStack, ply + 1);

        unmakeMove(board, m, u);
    }

    return nodes;
}

// Bestehende API bleibt: erzeugt nur einmal den Stack und ruft die schnelle Variante
uint64_t perft(Board& board, int depth) {
    // depth 10 => ply 0..10, also 11 Einträge reichen
    constexpr int MAX_PLY = 64;
    MoveList moveStack[MAX_PLY];

    return perft(board, depth, moveStack, 0);
}
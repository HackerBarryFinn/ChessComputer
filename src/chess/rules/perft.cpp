#include "chess/rules/perft.h"
#include "chess/core/movegen.h"
#include "chess/rules/makemove.h"

/**
 * Performs a perft (performance test) calculation to count all possible nodes at a given depth in a chess game tree.
 *
 * @param board The current state of the chess board.
 * @param depth The remaining depth to explore in the game tree.
 * @param moveStack A pointer to an array used to store move lists for each ply of the search.
 * @param ply The current ply (depth within the tree traversal).
 * @return The total number of nodes at the specified depth.
 */
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
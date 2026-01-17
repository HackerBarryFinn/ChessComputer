#include "chess/core/board.h"
#include <iostream>

/**
 * Initializes the bitboards, occupied squares, and king positions for both
 * players (White and Black) in a chess game. Sets up the starting positions
 * of all pieces on the board in the standard initial chess setup.
 *
 * @param board The `Board` object to be initialized. This includes:
 * - `bitboards` for the initial piece placements for each piece type.
 * - `occupied` arrays for tracking occupied squares for both players.
 * - `allOccupied` for all occupied squares.
 * - `kingSq` for the positions of the white and black kings.
 */
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

    // Neu: Königssquares initialisieren (e1=4, e8=60)
    board.kingSq[WHITE] = 4;
    board.kingSq[BLACK] = 60;
}

/**
 * Prints a visual representation of a given bitboard to the console.
 * The board is displayed with ranks (1-8) and files (a-h), where each square
 * shows "1" if there is a bit set in the corresponding position on the bitboard
 * and "." otherwise.
 *
 * @param bitboard The 64-bit unsigned integer representing the bitboard
 * where each bit corresponds to a square on the chessboard (from a1 to h8).
 */
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

/**
 * Prints the current state of the chessboard to the console. The board is
 * displayed in human-readable form with ranks and files labeled, and each piece
 * is represented by a specific character ('P' for white pawns, 'p' for black pawns,
 * and so on). Empty squares are displayed as '.'.
 *
 * @param board The `Board` object representing the current state of the game.
 * It includes:
 * - `bitboards` indicating piece positions for each type and color.
 * - Other data such as occupied squares and special game states.
 */
void printBoard(const Board &board) {
    for (int rank = 7; rank >= 0; --rank) {
        std::cout << rank + 1 << "  ";
        for (int file = 0; file < 8; ++file) {
            int square = rank * 8 + file;
            char pieceChar = '.';

            for (int color = WHITE; color <= BLACK; ++color) {
                for (int pt = PAWN; pt <= KING; ++pt) {
                    if ((board.bitboards[color][pt] >> square) & 1ULL) {
                        static constexpr char symbols[2][6] = {
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

#pragma once
#include <cstdint>
#include <array>

enum Color { WHITE = 0, BLACK = 1 };

enum PieceType { PAWN = 0, KNIGHT = 1, BISHOP = 2, ROOK = 3, QUEEN = 4, KING = 5 };

// Bitboard
struct Board {
    std::array<std::array<uint64_t, 6>, 2> bitboards{}; // [Color][PieceType]
    uint64_t occupied[2]{}; // WHITE, BLACK
    uint64_t allOccupied{}; // All pieces

    // Königssquares (0..63), -1 = nicht vorhanden
    int kingSq[2] = {-1, -1};

    Color sideToMove = WHITE;
    uint64_t enPassantTarget = 0ULL; // En passant target field
    bool whiteKingsideCastle = true;
    bool whiteQueensideCastle = true;
    bool blackKingsideCastle = true;
    bool blackQueensideCastle = true;
};

void initBitboards(Board &board);
void printBoard(const Board &board);

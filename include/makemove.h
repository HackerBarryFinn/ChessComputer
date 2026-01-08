#pragma once
#include "board.h"
#include "movegen.h"

struct UndoState {
    Color previousSideToMove;
    uint64_t previousEnPassantTarget;

    bool prevWhiteKingsideCastle;
    bool prevWhiteQueensideCastle;
    bool prevBlackKingsideCastle;
    bool prevBlackQueensideCastle;

    int prevKingSq[2] = {-1, -1};

    // Neu: Occupancy/AllOccupied fürs schnelle Undo (und als Sicherheitsnetz)
    uint64_t prevOccupied[2] = {0ULL, 0ULL};
    uint64_t prevAllOccupied = 0ULL;

    // Capture-Infos fürs Undo
    int capturedPiece = -1;     // PieceType als int, -1 = none
    int capturedSquare = -1;    // Square 0..63, -1 = none
};

bool makeMove(Board& board, const Move& move, UndoState& undo);
void unmakeMove(Board& board, const Move& move, const UndoState& undo);
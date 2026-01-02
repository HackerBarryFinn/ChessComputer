#pragma once
#include "board.h"
#include "movegen.h"
#include <cstdint>

struct UndoState {
    Color previousSideToMove;
    uint64_t previousEnPassantTarget;

    bool prevWhiteKingsideCastle;
    bool prevWhiteQueensideCastle;
    bool prevBlackKingsideCastle;
    bool prevBlackQueensideCastle;

    // Capture-Infos fürs Undo
    int capturedPiece = -1;     // PieceType als int, -1 = none
    int capturedSquare = -1;    // Square 0..63, -1 = none
};

// Führt einen Zug aus und füllt UndoState.
// Gibt false zurück, wenn der Zug nicht ausführbar ist (z.B. Quelle leer).
bool makeMove(Board& board, const Move& move, UndoState& undo);

// Macht makeMove rückgängig (muss mit dem UndoState aufgerufen werden).
void unmakeMove(Board& board, const Move& move, const UndoState& undo);
#pragma once
#include "board.h"
#include <string>

// Lädt eine Stellung aus FEN in 'board'.
// Unterstützt: Pieces, side to move, castling rights, en-passant square.
// Gibt true bei Erfolg zurück, sonst false (ungültige FEN).
bool loadFEN(Board& board, const std::string& fen);

#pragma once
#include "chess/core/board.h"
#include <string>

// Lädt eine Stellung aus FEN in 'board'.
// Gibt true bei Erfolg zurück, sonst false (ungültige FEN).
bool loadFEN(Board& board, const std::string& fen);

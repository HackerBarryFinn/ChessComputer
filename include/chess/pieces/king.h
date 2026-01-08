#pragma once

#include "chess/core/board.h"
#include "chess/core/movegen.h"
#include <vector>

void generateKingMoves(const Board& board, Color side, MoveList& out);

// Wrapper (alt)
std::vector<Move> generateKingMoves(const Board& board, Color side);
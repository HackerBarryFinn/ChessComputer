#pragma once

#include "chess/core/board.h"
#include "chess/core/movegen.h"
#include <vector>

void generateKnightMoves(const Board& board, Color side, MoveList& out);

// Wrapper (alt)
std::vector<Move> generateKnightMoves(const Board& board, Color side);
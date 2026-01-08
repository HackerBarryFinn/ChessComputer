#pragma once

#include "../board.h"
#include "../movegen.h"
#include <vector>

void generateKnightMoves(const Board& board, Color side, MoveList& out);

// Wrapper (alt)
std::vector<Move> generateKnightMoves(const Board& board, Color side);
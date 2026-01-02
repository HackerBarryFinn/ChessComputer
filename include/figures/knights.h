#pragma once

#include "../board.h"
#include "../movegen.h"
#include <vector>

std::vector<Move> generateKnightMoves(const Board& board, Color side);
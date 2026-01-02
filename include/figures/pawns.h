#pragma once

#include "../board.h"
#include "../movegen.h"
#include <vector>

std::vector<Move> generatePawnMoves(const Board& board, Color side);
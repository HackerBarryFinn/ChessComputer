#pragma once

#include "../board.h"
#include "../movegen.h"
#include <vector>

std::vector<Move> generateKingMoves(const Board& board, Color side);
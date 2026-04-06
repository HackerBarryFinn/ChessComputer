#pragma once

#include "chess/core/board.h"
#include "chess/core/movegen.h"

void generatePawnMoves(const Board& board, Color side, MoveList& out);
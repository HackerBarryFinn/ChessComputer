#pragma once

#include "chess/core/board.h"
#include "chess/core/movegen.h"

void generateKingMoves(const Board& board, Color side, MoveList& out);
#pragma once

#include "chess/core/board.h"
#include "chess/core/movegen.h"

void generateBishopMoves(const Board& board, Color side, MoveList& out);
void generateRookMoves(const Board& board, Color side, MoveList& out);
void generateQueenMoves(const Board& board, Color side, MoveList& out);
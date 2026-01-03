#pragma once

#include "../board.h"
#include "../movegen.h"
#include <vector>

std::vector<Move> generateBishopMoves(const Board& board, Color side);
std::vector<Move> generateRookMoves(const Board& board, Color side);
std::vector<Move> generateQueenMoves(const Board& board, Color side);
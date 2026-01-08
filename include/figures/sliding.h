#pragma once

#include "../board.h"
#include "../movegen.h"
#include <vector>

void generateBishopMoves(const Board& board, Color side, MoveList& out);
void generateRookMoves(const Board& board, Color side, MoveList& out);
void generateQueenMoves(const Board& board, Color side, MoveList& out);

// Wrapper (alt)
std::vector<Move> generateBishopMoves(const Board& board, Color side);
std::vector<Move> generateRookMoves(const Board& board, Color side);
std::vector<Move> generateQueenMoves(const Board& board, Color side);
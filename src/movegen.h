#pragma once
#include "board.h"
#include <vector>

struct Move {
    int from;
    int to;
    PieceType piece;
};

std::vector<Move> generatePawnMoves(const Board &board, Color side);

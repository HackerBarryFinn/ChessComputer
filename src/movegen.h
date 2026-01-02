#pragma once
#include "board.h"
#include <cstdint>
#include <vector>

enum MoveFlags : uint8_t {
    QUIET        = 0,
    CAPTURE      = 1 << 0,
    DOUBLE_PUSH  = 1 << 1,
    EN_PASSANT   = 1 << 2,
    CASTLING     = 1 << 3,
    PROMOTION    = 1 << 4
};

struct Move {
    int from = 0;
    int to = 0;

    PieceType moved = PAWN;

    // -1 bedeutet: kein Capture / keine Promotion
    int captured = -1;   // PieceType als int
    int promotion = -1;  // PieceType als int

    uint8_t flags = QUIET;
};

std::vector<Move> generatePawnMoves(const Board &board, Color side);

// zentraler Einstiegspunkt für später (Search/Perft/Legal-Filter)
std::vector<Move> generatePseudoLegalMoves(const Board &board, Color side);

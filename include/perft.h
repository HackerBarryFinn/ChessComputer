#pragma once
#include <cstdint>
#include "board.h"
#include "movegen.h"

// Bestehende API
uint64_t perft(Board& board, int depth);

// Neu: schnellere Variante mit MoveList-Stack (pro Ply wiederverwendet)
uint64_t perft(Board& board, int depth, MoveList* moveStack, int ply);
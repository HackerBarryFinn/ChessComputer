#pragma once
#include <cstdint>
#include "../core/board.h"
#include "../core/movegen.h"

// Bestehende API
uint64_t perft(Board& board, int depth);

// Neu: schnellere Variante mit MoveList-Stack (pro Ply wiederverwendet)
uint64_t perft(Board& board, int depth, MoveList* moveStack, int ply);
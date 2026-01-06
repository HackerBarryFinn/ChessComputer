#pragma once
#include "board.h"
#include "movegen.h"

// Liefert den besten gefundenen Zug für 'board.sideToMove' bei gegebener Tiefe.
Move findBestMove(Board& board, int depth);

// Debug: gibt die Top-N Root-Moves mit Bewertung aus (aus Sicht von board.sideToMove).
void printTopMoves(Board& board, int depth, int topN);

// Iterative Deepening: sucht depth=1..maxDepth und gibt je Iteration Bestmove/Score aus.
// Rückgabe: Bestmove der höchsten Tiefe.
Move findBestMoveIterative(Board& board, int maxDepth);
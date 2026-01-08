#pragma once

#include "../core/board.h"

// Gibt das Square (0..63) des Königs der Farbe zurück, oder -1 wenn nicht vorhanden.
int findKingSquare(const Board& board, Color side);

// true, wenn 'square' von 'byColor' angegriffen wird.
bool isSquareAttacked(const Board& board, int square, Color byColor);
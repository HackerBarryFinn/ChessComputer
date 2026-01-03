#include "../include/movegen.h"

#include "../include/figures/pawns.h"
#include "../include/figures/knights.h"
#include "../include/figures/king.h"
#include "../include/figures/sliding.h"

#include "../include/makemove.h"
#include "../include/attacks.h"

std::vector<Move> generatePseudoLegalMoves(const Board &board, Color side) {
    std::vector<Move> moves;

    auto pawnMoves = generatePawnMoves(board, side);
    moves.insert(moves.end(), pawnMoves.begin(), pawnMoves.end());

    auto knightMoves = generateKnightMoves(board, side);
    moves.insert(moves.end(), knightMoves.begin(), knightMoves.end());

    auto bishopMoves = generateBishopMoves(board, side);
    moves.insert(moves.end(), bishopMoves.begin(), bishopMoves.end());

    auto rookMoves = generateRookMoves(board, side);
    moves.insert(moves.end(), rookMoves.begin(), rookMoves.end());

    auto queenMoves = generateQueenMoves(board, side);
    moves.insert(moves.end(), queenMoves.begin(), queenMoves.end());

    auto kingMoves = generateKingMoves(board, side);
    moves.insert(moves.end(), kingMoves.begin(), kingMoves.end());

    return moves;
}

std::vector<Move> generateLegalMoves(Board &board, Color side) {
    std::vector<Move> legal;
    auto pseudo = generatePseudoLegalMoves(board, side);

    const Color enemy = (side == WHITE) ? BLACK : WHITE;

    for (const auto& m : pseudo) {
        UndoState u{};
        if (!makeMove(board, m, u)) {
            continue;
        }

        // König der ursprünglichen Seite darf nicht im Schach stehen
        int kingSq = findKingSquare(board, side);
        bool inCheck = (kingSq != -1) && isSquareAttacked(board, kingSq, enemy);

        unmakeMove(board, m, u);

        if (inCheck) continue;

        // Zusätzliche Rochade-Regeln: König darf nicht im oder über Schach rochieren
        if (m.flags & CASTLING) {
            // Startfeld ist immer e1/e8, Zwischenfeld f1/f8 oder d1/d8, Zielfeld g1/g8 oder c1/c8
            int e = (side == WHITE) ? 4  : 60;

            // Wenn König gar nicht auf e1/e8 stand
            if (m.from != e) continue;

            // König darf im Ausgangsfeld nicht im Schach sein
            if (isSquareAttacked(board, e, enemy)) continue;

            if (side == WHITE) {
                if (m.to == 6) { // g1
                    if (isSquareAttacked(board, 5, enemy)) continue; // f1
                    if (isSquareAttacked(board, 6, enemy)) continue; // g1
                } else if (m.to == 2) { // c1
                    if (isSquareAttacked(board, 3, enemy)) continue; // d1
                    if (isSquareAttacked(board, 2, enemy)) continue; // c1
                } else continue;
            } else {
                if (m.to == 62) { // g8
                    if (isSquareAttacked(board, 61, enemy)) continue; // f8
                    if (isSquareAttacked(board, 62, enemy)) continue; // g8
                } else if (m.to == 58) { // c8
                    if (isSquareAttacked(board, 59, enemy)) continue; // d8
                    if (isSquareAttacked(board, 58, enemy)) continue; // c8
                } else continue;
            }
        }

        legal.push_back(m);
    }

    return legal;
}
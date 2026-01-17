#include "chess/core/movegen.h"

#include "chess/pieces/pawns.h"
#include "chess/pieces/knights.h"
#include "chess/pieces/king.h"
#include "chess/pieces/sliding.h"

#include "chess/rules/makemove.h"
#include "chess/rules/attacks.h"

/**
 * Generates all pseudo-legal moves for a given board state and side to move.
 * Pseudo-legal moves do not account for the king being in check or moving through attacked squares.
 *
 * @param board The current state of the chess board.
 * @param side The side (color) for which moves should be generated (WHITE or BLACK).
 * @param out A MoveList structure where the generated moves will be stored. This will be cleared and populated with the new moves.
 */
void generatePseudoLegalMoves(const Board &board, Color side, MoveList& out) {
    out.clear();

    generatePawnMoves(board, side, out);
    generateKnightMoves(board, side, out);
    generateBishopMoves(board, side, out);
    generateRookMoves(board, side, out);
    generateQueenMoves(board, side, out);
    generateKingMoves(board, side, out);
}

/**
 * Generates all legal moves for a given board state and side to move. Legal moves ensure that
 * the king is not in check after the move and adhere to all chess rules such as castling
 * restrictions.
 *
 * @param board The current state of the chess board.
 * @param side The side (color) for which legal moves should be generated (WHITE or BLACK).
 * @param out A MoveList structure where the validated legal moves will be stored. This will be cleared and populated with the new moves.
 */
void generateLegalMoves(Board &board, Color side, MoveList& out) {
    // 1) pseudo-legal direkt in out erzeugen (kein zweiter Buffer)
    generatePseudoLegalMoves(board, side, out);

    const Color enemy = (side == WHITE) ? BLACK : WHITE;

    // 2) In-place filtern: read i, write w
    int w = 0;
    for (int i = 0; i < out.size; ++i) {
        const Move m = out[i]; // Kopie ist ok (out wird überschrieben)

        UndoState u{};
        if (!makeMove(board, m, u)) continue;

        // König der ursprünglichen Seite darf nicht im Schach stehen
        const int kingSq = board.kingSq[side];
        const bool inCheck = (kingSq != -1) && isSquareAttacked(board, kingSq, enemy);

        unmakeMove(board, m, u);

        if (inCheck) continue;

        // Zusätzliche Rochade-Regeln: König darf nicht im oder über Schach rochieren
        if (m.flags & CASTLING) {
            const int e = (side == WHITE) ? 4 : 60;
            if (m.from != e) continue;

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

        out[w++] = m;
    }

    out.size = w;
}

// ------- Wrapper (alt) -------

/**
 * Generates all pseudo-legal moves for the given board state and side to move.
 * Pseudo-legal moves are moves that do not consider whether the king is in check
 * or moves through attacked squares.
 *
 * @param board The current state of the chess board.
 * @param side The side (color) for which moves should be generated (WHITE or BLACK).
 * @return A vector containing all pseudo-legal moves for the given side.
 */
std::vector<Move> generatePseudoLegalMoves(const Board &board, Color side) {
    MoveList tmp;
    generatePseudoLegalMoves(board, side, tmp);
    return std::vector<Move>(tmp.data.begin(), tmp.data.begin() + tmp.size);
}

/**
 * Generates all legal moves for a given board state and side to move.
 * Legal moves ensure that the player's king is not in check after the move.
 *
 * @param board The current state of the chess board.
 * @param side The side (color) for which legal moves should be generated (WHITE or BLACK).
 * @return A vector containing all legal moves for the specified side on the given board.
 */
std::vector<Move> generateLegalMoves(Board &board, Color side) {
    MoveList tmp;
    generateLegalMoves(board, side, tmp);
    return std::vector<Move>(tmp.data.begin(), tmp.data.begin() + tmp.size);
}
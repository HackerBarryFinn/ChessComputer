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

        // Neu: cached king square statt findKingSquare()
        int kingSq = board.kingSq[side];
        bool inCheck = (kingSq != -1) && isSquareAttacked(board, kingSq, enemy);

        unmakeMove(board, m, u);

        if (inCheck) continue;

        if (m.flags & CASTLING) {
            int e = (side == WHITE) ? 4  : 60;
            if (m.from != e) continue;

            if (isSquareAttacked(board, e, enemy)) continue;

            if (side == WHITE) {
                if (m.to == 6) {
                    if (isSquareAttacked(board, 5, enemy)) continue;
                    if (isSquareAttacked(board, 6, enemy)) continue;
                } else if (m.to == 2) {
                    if (isSquareAttacked(board, 3, enemy)) continue;
                    if (isSquareAttacked(board, 2, enemy)) continue;
                } else continue;
            } else {
                if (m.to == 62) {
                    if (isSquareAttacked(board, 61, enemy)) continue;
                    if (isSquareAttacked(board, 62, enemy)) continue;
                } else if (m.to == 58) {
                    if (isSquareAttacked(board, 59, enemy)) continue;
                    if (isSquareAttacked(board, 58, enemy)) continue;
                } else continue;
            }
        }

        legal.push_back(m);
    }

    return legal;
}
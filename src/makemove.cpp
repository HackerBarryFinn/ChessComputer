#include "../include/makemove.h"

static inline uint64_t sqBB(int sq) { return 1ULL << sq; }

static void recomputeOccupancy(Board& b) {
    b.occupied[WHITE] = 0ULL;
    b.occupied[BLACK] = 0ULL;
    for (int pt = PAWN; pt <= KING; ++pt) {
        b.occupied[WHITE] |= b.bitboards[WHITE][pt];
        b.occupied[BLACK] |= b.bitboards[BLACK][pt];
    }
    b.allOccupied = b.occupied[WHITE] | b.occupied[BLACK];
}

static int findPieceOnSquare(const Board& b, Color c, int square) {
    uint64_t mask = sqBB(square);
    for (int pt = PAWN; pt <= KING; ++pt) {
        if (b.bitboards[c][pt] & mask) return pt;
    }
    return -1;
}

bool makeMove(Board& board, const Move& move, UndoState& undo) {
    // Undo sichern
    undo.previousSideToMove = board.sideToMove;
    undo.previousEnPassantTarget = board.enPassantTarget;

    undo.prevWhiteKingsideCastle = board.whiteKingsideCastle;
    undo.prevWhiteQueensideCastle = board.whiteQueensideCastle;
    undo.prevBlackKingsideCastle = board.blackKingsideCastle;
    undo.prevBlackQueensideCastle = board.blackQueensideCastle;

    undo.capturedPiece = -1;
    undo.capturedSquare = -1;

    Color side = board.sideToMove;
    Color enemy = (side == WHITE) ? BLACK : WHITE;

    const uint64_t fromBB = sqBB(move.from);
    const uint64_t toBB   = sqBB(move.to);

    // Minimaler Plausibilitätscheck: steht die Figur am from?
    if ((board.bitboards[side][move.moved] & fromBB) == 0ULL) {
        return false;
    }

    // Standard: en-passant Target wird nach jedem Zug gelöscht,
    // nur bei DOUBLE_PUSH neu gesetzt.
    board.enPassantTarget = 0ULL;

    // 1) Captures (inkl. EP)
    if (move.flags & EN_PASSANT) {
        // EP: Zielfeld ist leer, geschlagen wird der Bauer "hinter" dem Zielfeld
        int capSq = (side == WHITE) ? (move.to - 8) : (move.to + 8);
        uint64_t capBB = sqBB(capSq);

        // es muss ein gegnerischer Bauer dort stehen
        if ((board.bitboards[enemy][PAWN] & capBB) == 0ULL) {
            return false;
        }

        board.bitboards[enemy][PAWN] &= ~capBB;
        undo.capturedPiece = PAWN;
        undo.capturedSquare = capSq;
    } else if (move.flags & CAPTURE) {
        // normales Capture auf dem Zielfeld
        int captured = findPieceOnSquare(board, enemy, move.to);
        if (captured == -1) {
            return false; // nichts zu schlagen -> inkonsistent
        }
        board.bitboards[enemy][captured] &= ~toBB;
        undo.capturedPiece = captured;
        undo.capturedSquare = move.to;
    }

    // 2) Figur bewegen (Pawn)
    // Pawn vom from entfernen
    board.bitboards[side][move.moved] &= ~fromBB;

    // Promotion?
    if (move.flags & PROMOTION) {
        if (move.promotion < PAWN || move.promotion > KING) return false;
        board.bitboards[side][static_cast<PieceType>(move.promotion)] |= toBB;
    } else {
        board.bitboards[side][move.moved] |= toBB;
    }

    // 3) Double Push -> enPassantTarget setzen
    if (move.flags & DOUBLE_PUSH) {
        // Ziel ist 2 Felder vor; EP-Ziel ist das Feld dazwischen
        int epSq = (side == WHITE) ? (move.from + 8) : (move.from - 8);
        board.enPassantTarget = sqBB(epSq);
    }

    // Castling rights ändern wir für Pawn-Moves nicht (später bei King/Rook)

    // side to move wechseln + Occupancy neu berechnen
    board.sideToMove = enemy;
    recomputeOccupancy(board);
    return true;
}

void unmakeMove(Board& board, const Move& move, const UndoState& undo) {
    // State zurück
    board.sideToMove = undo.previousSideToMove;
    board.enPassantTarget = undo.previousEnPassantTarget;

    board.whiteKingsideCastle = undo.prevWhiteKingsideCastle;
    board.whiteQueensideCastle = undo.prevWhiteQueensideCastle;
    board.blackKingsideCastle = undo.prevBlackKingsideCastle;
    board.blackQueensideCastle = undo.prevBlackQueensideCastle;

    Color side = board.sideToMove;
    Color enemy = (side == WHITE) ? BLACK : WHITE;

    const uint64_t fromBB = sqBB(move.from);
    const uint64_t toBB   = sqBB(move.to);

    // 1) Figur zurück bewegen
    if (move.flags & PROMOTION) {
        // promoted piece vom 'to' entfernen, pawn wieder auf from
        board.bitboards[side][static_cast<PieceType>(move.promotion)] &= ~toBB;
        board.bitboards[side][PAWN] |= fromBB;
    } else {
        // moved piece vom 'to' entfernen, wieder auf from
        board.bitboards[side][move.moved] &= ~toBB;
        board.bitboards[side][move.moved] |= fromBB;
    }

    // 2) Capture rückgängig machen
    if (undo.capturedPiece != -1) {
        board.bitboards[enemy][undo.capturedPiece] |= sqBB(undo.capturedSquare);
    }

    recomputeOccupancy(board);
}
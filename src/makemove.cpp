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

static void revokeCastlingRightsForMove(Board& board, Color side, const Move& move) {
    // Wenn König zieht -> beide Rechte weg
    if (move.moved == KING) {
        if (side == WHITE) {
            board.whiteKingsideCastle = false;
            board.whiteQueensideCastle = false;
        } else {
            board.blackKingsideCastle = false;
            board.blackQueensideCastle = false;
        }
        return;
    }

    // Wenn Turm von Startfeld zieht -> entsprechendes Recht weg
    if (move.moved == ROOK) {
        if (side == WHITE) {
            if (move.from == 0) board.whiteQueensideCastle = false; // a1
            if (move.from == 7) board.whiteKingsideCastle = false;  // h1
        } else {
            if (move.from == 56) board.blackQueensideCastle = false; // a8
            if (move.from == 63) board.blackKingsideCastle = false;  // h8
        }
    }
}

static void revokeCastlingRightsForCapturedRook(Board& board, Color enemy, int capturedSquare, int capturedPiece) {
    if (capturedPiece != ROOK) return;

    if (enemy == WHITE) {
        if (capturedSquare == 0) board.whiteQueensideCastle = false;
        if (capturedSquare == 7) board.whiteKingsideCastle = false;
    } else {
        if (capturedSquare == 56) board.blackQueensideCastle = false;
        if (capturedSquare == 63) board.blackKingsideCastle = false;
    }
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

    if ((board.bitboards[side][move.moved] & fromBB) == 0ULL) {
        return false;
    }

    // Merken, was vor dem Zug gültig war (EP muss gegen den "alten" Zustand geprüft werden)
    const uint64_t oldEpTarget = board.enPassantTarget;

    // Standard: EP-Target wird nach jedem Zug gelöscht,
    // nur bei Pawn DOUBLE_PUSH neu gesetzt.
    board.enPassantTarget = 0ULL;

    // 1) Captures (inkl. EP)
    if (move.flags & EN_PASSANT) {
        // EP ist nur für Bauern, und darf nicht mit anderen Sonderflags kombiniert werden
        if (move.moved != PAWN) { board.enPassantTarget = oldEpTarget; return false; }
        if (move.flags & (DOUBLE_PUSH | PROMOTION | CASTLING)) { board.enPassantTarget = oldEpTarget; return false; }

        // EP muss auf das EP-Target-Feld gehen (aus dem Zustand VOR dem Zug)
        if (oldEpTarget == 0ULL) { board.enPassantTarget = oldEpTarget; return false; }
        if (oldEpTarget != toBB) { board.enPassantTarget = oldEpTarget; return false; }

        // Zielfeld muss leer sein (EP schlägt nicht die Figur auf "to", sondern daneben)
        if (board.allOccupied & toBB) { board.enPassantTarget = oldEpTarget; return false; }

        // Bewegung muss diagonal sein
        int delta = move.to - move.from;
        if (side == WHITE) {
            if (!(delta == 7 || delta == 9)) { board.enPassantTarget = oldEpTarget; return false; }
        } else {
            if (!(delta == -7 || delta == -9)) { board.enPassantTarget = oldEpTarget; return false; }
        }

        // geschlagener Bauer steht "hinter" dem Ziel
        int capSq = (side == WHITE) ? (move.to - 8) : (move.to + 8);
        uint64_t capBB = sqBB(capSq);

        if ((board.bitboards[enemy][PAWN] & capBB) == 0ULL) {
            board.enPassantTarget = oldEpTarget;
            return false;
        }

        board.bitboards[enemy][PAWN] &= ~capBB;
        undo.capturedPiece = PAWN;
        undo.capturedSquare = capSq;
    } else if (move.flags & CAPTURE) {
        int captured = findPieceOnSquare(board, enemy, move.to);
        if (captured == -1) {
            return false;
        }
        board.bitboards[enemy][captured] &= ~toBB;
        undo.capturedPiece = captured;
        undo.capturedSquare = move.to;
    }

    // 2) Figur bewegen (inkl. Promotion)
    board.bitboards[side][move.moved] &= ~fromBB;

    if (move.moved == PAWN && (move.flags & PROMOTION)) {
        if (move.promotion < PAWN || move.promotion > KING) return false;
        board.bitboards[side][static_cast<PieceType>(move.promotion)] |= toBB;
    } else {
        board.bitboards[side][move.moved] |= toBB;
    }

    // 2b) Rochade: Turm mitziehen (King zieht bereits von->to)
    if (move.flags & CASTLING) {
        // Wir erwarten: moved == KING
        if (move.moved != KING) return false;

        // Zusätzliche Robustheits-Checks:
        // - Turm muss wirklich auf dem Startfeld stehen
        // - Zwischenfelder müssen frei sein (entspricht den Regeln + schützt Perft gegen falsche Gen)
        if (side == WHITE) {
            if (move.from == 4 && move.to == 6) {
                // e1->g1: rook h1->f1, Felder f1/g1 müssen frei sein
                if ((board.bitboards[WHITE][ROOK] & sqBB(7)) == 0ULL) return false;
                if (board.allOccupied & (sqBB(5) | sqBB(6))) return false;

                board.bitboards[WHITE][ROOK] &= ~sqBB(7);
                board.bitboards[WHITE][ROOK] |=  sqBB(5);
            } else if (move.from == 4 && move.to == 2) {
                // e1->c1: rook a1->d1, Felder b1/c1/d1 müssen frei sein
                if ((board.bitboards[WHITE][ROOK] & sqBB(0)) == 0ULL) return false;
                if (board.allOccupied & (sqBB(1) | sqBB(2) | sqBB(3))) return false;

                board.bitboards[WHITE][ROOK] &= ~sqBB(0);
                board.bitboards[WHITE][ROOK] |=  sqBB(3);
            } else {
                return false;
            }
        } else {
            if (move.from == 60 && move.to == 62) {
                // e8->g8: rook h8->f8, Felder f8/g8 müssen frei sein
                if ((board.bitboards[BLACK][ROOK] & sqBB(63)) == 0ULL) return false;
                if (board.allOccupied & (sqBB(61) | sqBB(62))) return false;

                board.bitboards[BLACK][ROOK] &= ~sqBB(63);
                board.bitboards[BLACK][ROOK] |=  sqBB(61);
            } else if (move.from == 60 && move.to == 58) {
                // e8->c8: rook a8->d8, Felder b8/c8/d8 müssen frei sein
                if ((board.bitboards[BLACK][ROOK] & sqBB(56)) == 0ULL) return false;
                if (board.allOccupied & (sqBB(57) | sqBB(58) | sqBB(59))) return false;

                board.bitboards[BLACK][ROOK] &= ~sqBB(56);
                board.bitboards[BLACK][ROOK] |=  sqBB(59);
            } else {
                return false;
            }
        }
    }

    // 3) Pawn Double Push -> enPassantTarget setzen
    if (move.flags & DOUBLE_PUSH) {
        if (move.moved != PAWN) return false;
        if (move.flags & (CAPTURE | EN_PASSANT | PROMOTION | CASTLING)) return false;

        int delta = move.to - move.from;
        int fromRank = move.from / 8;

        // Startreihe + Delta prüfen
        if (side == WHITE) {
            if (fromRank != 1) return false;
            if (delta != 16) return false;
        } else {
            if (fromRank != 6) return false;
            if (delta != -16) return false;
        }

        // Zwischenfeld + Zielfeld müssen frei sein
        int midSq = (side == WHITE) ? (move.from + 8) : (move.from - 8);
        if (board.allOccupied & (sqBB(midSq) | sqBB(move.to))) return false;

        // EP-Target korrekt setzen
        board.enPassantTarget = sqBB(midSq);
    }

    // 4) Castling rights updaten (King/Rook move + evtl. rook capture)
    revokeCastlingRightsForMove(board, side, move);
    if (undo.capturedPiece != -1) {
        revokeCastlingRightsForCapturedRook(board, enemy, undo.capturedSquare, undo.capturedPiece);
    }

    board.sideToMove = enemy;
    recomputeOccupancy(board);
    return true;
}

void unmakeMove(Board& board, const Move& move, const UndoState& undo) {
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

    // 1) Rochade rückgängig: Turm zurückziehen (King machen wir weiter unten)
    if (move.flags & CASTLING) {
        if (move.moved != KING) {
            // sollte nicht passieren
        } else if (side == WHITE) {
            if (move.from == 4 && move.to == 6) {
                board.bitboards[WHITE][ROOK] &= ~sqBB(5);
                board.bitboards[WHITE][ROOK] |=  sqBB(7);
            } else if (move.from == 4 && move.to == 2) {
                board.bitboards[WHITE][ROOK] &= ~sqBB(3);
                board.bitboards[WHITE][ROOK] |=  sqBB(0);
            }
        } else {
            if (move.from == 60 && move.to == 62) {
                board.bitboards[BLACK][ROOK] &= ~sqBB(61);
                board.bitboards[BLACK][ROOK] |=  sqBB(63);
            } else if (move.from == 60 && move.to == 58) {
                board.bitboards[BLACK][ROOK] &= ~sqBB(59);
                board.bitboards[BLACK][ROOK] |=  sqBB(56);
            }
        }
    }

    // 2) Figur zurück bewegen
    if (move.moved == PAWN && (move.flags & PROMOTION)) {
        board.bitboards[side][static_cast<PieceType>(move.promotion)] &= ~toBB;
        board.bitboards[side][PAWN] |= fromBB;
    } else {
        board.bitboards[side][move.moved] &= ~toBB;
        board.bitboards[side][move.moved] |= fromBB;
    }

    // 3) Capture rückgängig machen
    if (undo.capturedPiece != -1) {
        board.bitboards[enemy][undo.capturedPiece] |= sqBB(undo.capturedSquare);
    }

    recomputeOccupancy(board);
}
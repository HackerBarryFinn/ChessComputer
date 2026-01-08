#include "../include/makemove.h"

static inline uint64_t sqBB(int sq) { return 1ULL << sq; }

// recomputeOccupancy wird nicht mehr benötigt (kann später gelöscht werden)
// static void recomputeOccupancy(Board& b) { ... }

static int findPieceOnSquare(const Board& b, Color c, int square) {
    uint64_t mask = sqBB(square);
    for (int pt = PAWN; pt <= KING; ++pt) {
        if (b.bitboards[c][pt] & mask) return pt;
    }
    return -1;
}

static void revokeCastlingRightsForMove(Board& board, Color side, const Move& move) {
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

    if (move.moved == ROOK) {
        if (side == WHITE) {
            if (move.from == 0) board.whiteQueensideCastle = false;
            if (move.from == 7) board.whiteKingsideCastle = false;
        } else {
            if (move.from == 56) board.blackQueensideCastle = false;
            if (move.from == 63) board.blackKingsideCastle = false;
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

    undo.prevKingSq[WHITE] = board.kingSq[WHITE];
    undo.prevKingSq[BLACK] = board.kingSq[BLACK];

    // Neu: Occupancy sichern
    undo.prevOccupied[WHITE] = board.occupied[WHITE];
    undo.prevOccupied[BLACK] = board.occupied[BLACK];
    undo.prevAllOccupied = board.allOccupied;

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

    // Wichtig: Viele eurer Validierungen prüfen absichtlich gegen den "alten" Zustand.
    const uint64_t oldAllOccupied = board.allOccupied;

    // Standard: EP-Target wird nach jedem Zug gelöscht,
    // nur bei Pawn DOUBLE_PUSH neu gesetzt.
    board.enPassantTarget = 0ULL;

    // 1) Captures (inkl. EP)
    if (move.flags & EN_PASSANT) {
        if (move.moved != PAWN) { board.enPassantTarget = oldEpTarget; return false; }
        if (move.flags & (DOUBLE_PUSH | PROMOTION | CASTLING)) { board.enPassantTarget = oldEpTarget; return false; }

        if (oldEpTarget == 0ULL) { board.enPassantTarget = oldEpTarget; return false; }
        if (oldEpTarget != toBB) { board.enPassantTarget = oldEpTarget; return false; }

        // Zielfeld muss im alten Zustand leer sein
        if (oldAllOccupied & toBB) { board.enPassantTarget = oldEpTarget; return false; }

        int delta = move.to - move.from;
        if (side == WHITE) {
            if (!(delta == 7 || delta == 9)) { board.enPassantTarget = oldEpTarget; return false; }
        } else {
            if (!(delta == -7 || delta == -9)) { board.enPassantTarget = oldEpTarget; return false; }
        }

        int capSq = (side == WHITE) ? (move.to - 8) : (move.to + 8);
        uint64_t capBB = sqBB(capSq);

        if ((board.bitboards[enemy][PAWN] & capBB) == 0ULL) {
            board.enPassantTarget = oldEpTarget;
            return false;
        }

        board.bitboards[enemy][PAWN] &= ~capBB;
        // Neu: Occupancy inkrementell
        board.occupied[enemy] &= ~capBB;

        undo.capturedPiece = PAWN;
        undo.capturedSquare = capSq;
    } else if (move.flags & CAPTURE) {
        int captured = findPieceOnSquare(board, enemy, move.to);
        if (captured == -1) {
            return false;
        }
        board.bitboards[enemy][captured] &= ~toBB;
        // Neu: Occupancy inkrementell
        board.occupied[enemy] &= ~toBB;

        undo.capturedPiece = captured;
        undo.capturedSquare = move.to;
    }

    // 2) Figur bewegen (inkl. Promotion)
    board.bitboards[side][move.moved] &= ~fromBB;
    board.occupied[side] &= ~fromBB; // Neu

    if (move.moved == PAWN && (move.flags & PROMOTION)) {
        if (move.promotion < PAWN || move.promotion > KING) return false;
        board.bitboards[side][static_cast<PieceType>(move.promotion)] |= toBB;
        // Occupancy: Zielfeld ist belegt (egal welche Figur)
        board.occupied[side] |= toBB; // Neu
    } else {
        board.bitboards[side][move.moved] |= toBB;
        board.occupied[side] |= toBB; // Neu
    }

    if (move.moved == KING) {
        // robuster als board.sideToMove (hier garantiert die ziehende Farbe)
        board.kingSq[side] = move.to;
    }

    // 2b) Rochade: Turm mitziehen (King zieht bereits von->to)
    if (move.flags & CASTLING) {
        if (move.moved != KING) return false;

        if (side == WHITE) {
            if (move.from == 4 && move.to == 6) {
                if ((board.bitboards[WHITE][ROOK] & sqBB(7)) == 0ULL) return false;
                if (oldAllOccupied & (sqBB(5) | sqBB(6))) return false;

                board.bitboards[WHITE][ROOK] &= ~sqBB(7);
                board.bitboards[WHITE][ROOK] |=  sqBB(5);

                // Neu: Occupancy Turm inkrementell
                board.occupied[WHITE] &= ~sqBB(7);
                board.occupied[WHITE] |=  sqBB(5);
            } else if (move.from == 4 && move.to == 2) {
                if ((board.bitboards[WHITE][ROOK] & sqBB(0)) == 0ULL) return false;
                if (oldAllOccupied & (sqBB(1) | sqBB(2) | sqBB(3))) return false;

                board.bitboards[WHITE][ROOK] &= ~sqBB(0);
                board.bitboards[WHITE][ROOK] |=  sqBB(3);

                board.occupied[WHITE] &= ~sqBB(0);
                board.occupied[WHITE] |=  sqBB(3);
            } else {
                return false;
            }
        } else {
            if (move.from == 60 && move.to == 62) {
                if ((board.bitboards[BLACK][ROOK] & sqBB(63)) == 0ULL) return false;
                if (oldAllOccupied & (sqBB(61) | sqBB(62))) return false;

                board.bitboards[BLACK][ROOK] &= ~sqBB(63);
                board.bitboards[BLACK][ROOK] |=  sqBB(61);

                board.occupied[BLACK] &= ~sqBB(63);
                board.occupied[BLACK] |=  sqBB(61);
            } else if (move.from == 60 && move.to == 58) {
                if ((board.bitboards[BLACK][ROOK] & sqBB(56)) == 0ULL) return false;
                if (oldAllOccupied & (sqBB(57) | sqBB(58) | sqBB(59))) return false;

                board.bitboards[BLACK][ROOK] &= ~sqBB(56);
                board.bitboards[BLACK][ROOK] |=  sqBB(59);

                board.occupied[BLACK] &= ~sqBB(56);
                board.occupied[BLACK] |=  sqBB(59);
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

        if (side == WHITE) {
            if (fromRank != 1) return false;
            if (delta != 16) return false;
        } else {
            if (fromRank != 6) return false;
            if (delta != -16) return false;
        }

        int midSq = (side == WHITE) ? (move.from + 8) : (move.from - 8);

        // Hier ebenfalls gegen alten Zustand prüfen
        if (oldAllOccupied & (sqBB(midSq) | sqBB(move.to))) return false;

        board.enPassantTarget = sqBB(midSq);
    }

    // 4) Castling rights updaten
    revokeCastlingRightsForMove(board, side, move);
    if (undo.capturedPiece != -1) {
        revokeCastlingRightsForCapturedRook(board, enemy, undo.capturedSquare, undo.capturedPiece);
    }

    board.sideToMove = enemy;

    // Neu: allOccupied billig neu setzen (nur OR der beiden Farben)
    board.allOccupied = board.occupied[WHITE] | board.occupied[BLACK];

    return true;
}

void unmakeMove(Board& board, const Move& move, const UndoState& undo) {
    (void)move; // move wird weiterhin benutzt, aber falls dein Compiler warnt, kann das weg

    board.kingSq[WHITE] = undo.prevKingSq[WHITE];
    board.kingSq[BLACK] = undo.prevKingSq[BLACK];

    board.sideToMove = undo.previousSideToMove;
    board.enPassantTarget = undo.previousEnPassantTarget;

    board.whiteKingsideCastle = undo.prevWhiteKingsideCastle;
    board.whiteQueensideCastle = undo.prevWhiteQueensideCastle;
    board.blackKingsideCastle = undo.prevBlackKingsideCastle;
    board.blackQueensideCastle = undo.prevBlackQueensideCastle;

    // Bitboards wie bisher zurückbauen (dein bestehender Code bleibt),
    // ABER: Occupancy nicht mehr recompute'n, sondern direkt wiederherstellen:
    // (Wir stellen das absichtlich am Ende wieder her, damit Bitboards-Undo-Code unverändert bleiben kann.)

    Color side = board.sideToMove;
    Color enemy = (side == WHITE) ? BLACK : WHITE;

    const uint64_t fromBB = sqBB(move.from);
    const uint64_t toBB   = sqBB(move.to);

    if (move.flags & CASTLING) {
        if (move.moved == KING) {
            if (side == WHITE) {
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
    }

    if (move.moved == PAWN && (move.flags & PROMOTION)) {
        board.bitboards[side][static_cast<PieceType>(move.promotion)] &= ~toBB;
        board.bitboards[side][PAWN] |= fromBB;
    } else {
        board.bitboards[side][move.moved] &= ~toBB;
        board.bitboards[side][move.moved] |= fromBB;
    }

    if (undo.capturedPiece != -1) {
        board.bitboards[enemy][undo.capturedPiece] |= sqBB(undo.capturedSquare);
    }

    // Neu: Occupancy/AllOccupied exakt wiederherstellen (kein recompute)
    board.occupied[WHITE] = undo.prevOccupied[WHITE];
    board.occupied[BLACK] = undo.prevOccupied[BLACK];
    board.allOccupied = undo.prevAllOccupied;
}
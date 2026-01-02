#include <iostream>
#include <string>
#include "../include/board.h"
#include "../include/movegen.h"
#include "../include/fen.h"
#include "../include/makemove.h"

static std::string squareToString(int sq) {
    int file = sq % 8;
    int rank = sq / 8;
    std::string s;
    s += static_cast<char>('a' + file);
    s += static_cast<char>('1' + rank);
    return s;
}

static std::string flagsToString(uint8_t flags) {
    std::string out;
    if (flags == QUIET) out += "QUIET|";
    if (flags & CAPTURE) out += "CAPTURE|";
    if (flags & DOUBLE_PUSH) out += "DOUBLE_PUSH|";
    if (flags & EN_PASSANT) out += "EN_PASSANT|";
    if (flags & CASTLING) out += "CASTLING|";
    if (flags & PROMOTION) out += "PROMOTION|";
    if (!out.empty()) out.pop_back();
    return out;
}

static char pieceToChar(int pt) {
    switch (pt) {
        case PAWN: return 'P';
        case KNIGHT: return 'N';
        case BISHOP: return 'B';
        case ROOK: return 'R';
        case QUEEN: return 'Q';
        case KING: return 'K';
        default: return '-';
    }
}

static bool boardsEqual(const Board& a, const Board& b) {
    if (a.bitboards != b.bitboards) return false;

    if (a.occupied[WHITE] != b.occupied[WHITE]) return false;
    if (a.occupied[BLACK] != b.occupied[BLACK]) return false;
    if (a.allOccupied != b.allOccupied) return false;

    if (a.sideToMove != b.sideToMove) return false;
    if (a.enPassantTarget != b.enPassantTarget) return false;

    if (a.whiteKingsideCastle != b.whiteKingsideCastle) return false;
    if (a.whiteQueensideCastle != b.whiteQueensideCastle) return false;
    if (a.blackKingsideCastle != b.blackKingsideCastle) return false;
    if (a.blackQueensideCastle != b.blackQueensideCastle) return false;

    return true;
}

static void printMove(const Move& m) {
    std::cout << squareToString(m.from) << squareToString(m.to)
              << "  flags=" << flagsToString(m.flags);

    if (m.flags & PROMOTION) {
        std::cout << "  promo=" << pieceToChar(m.promotion);
    }
    if (m.flags & CAPTURE) {
        std::cout << "  cap=" << pieceToChar(m.captured);
    }
    std::cout << "\n";
}

static void runTest(const std::string& name, const std::string& fen, Color side) {
    Board b;
    if (!loadFEN(b, fen)) {
        std::cout << "[FAIL] " << name << ": FEN konnte nicht geladen werden\n";
        return;
    }

    auto moves = generatePawnMoves(b, side);

    std::cout << "\n=== " << name << " ===\n";
    std::cout << "FEN: " << fen << "\n";
    std::cout << "Pawn moves: " << moves.size() << "\n";

    for (const auto& m : moves) {
        printMove(m);
    }

    std::cout << "Roundtrip (make/unmake) + enPassantTarget Checks...\n";
    const Board original = b;

    int ok = 0;
    for (const auto& m : moves) {
        UndoState u{};
        if (!makeMove(b, m, u)) {
            std::cout << "[FAIL] makeMove fehlgeschlagen bei Move: ";
            printMove(m);
            return;
        }

        // Zusatzcheck: enPassantTarget nach makeMove
        if (m.flags & DOUBLE_PUSH) {
            if (b.enPassantTarget == 0ULL) {
                std::cout << "[FAIL] DOUBLE_PUSH aber enPassantTarget == 0 bei Move: ";
                printMove(m);
                return;
            }
        } else {
            if (b.enPassantTarget != 0ULL) {
                std::cout << "[FAIL] enPassantTarget wurde gesetzt, obwohl kein DOUBLE_PUSH bei Move: ";
                printMove(m);
                return;
            }
        }

        unmakeMove(b, m, u);

        if (!boardsEqual(b, original)) {
            std::cout << "[FAIL] Board nach unmakeMove ungleich original bei Move: ";
            printMove(m);
            std::cout << "Original Board:\n";
            printBoard(original);
            std::cout << "Aktuelles Board:\n";
            printBoard(b);
            return;
        }
        ok++;
    }

    std::cout << "[OK] Roundtrip+Checks bestanden für " << ok << " Moves.\n";
}

int main() {
    runTest(
        "Promotion (white pawn a7->a8)",
        "8/P7/8/8/8/8/8/4k3 w - - 0 1",
        WHITE
    );

    runTest(
        "En passant (white e5xd6 ep)",
        "8/8/8/3pP3/8/8/8/4k3 w - d6 0 1",
        WHITE
    );

    // Normal Capture: Weißer Bauer e4 kann f5 schlagen (schwarzer Bauer auf f5)
    runTest(
        "Normal capture (white e4xf5)",
        "8/8/8/5p2/4P3/8/8/4k3 w - - 0 1",
        WHITE
    );

    // Double Push: Weißer Bauer e2 kann e4 (doppel) und e3 (einfach)
    runTest(
        "Double push (white e2->e4 sets EP square e3)",
        "8/8/8/8/8/8/4P3/4k3 w - - 0 1",
        WHITE
    );

    // Black Promotion: schwarzer Bauer a2 -> a1 (Q,R,B,N)
    runTest(
        "Promotion (black pawn a2->a1)",
        "8/8/8/8/8/8/p7/4K3 b - - 0 1",
        BLACK
    );

    // Black En-passant:
    // Weißer Bauer d4, schwarzer Bauer e4, EP-Ziel ist d3 -> Schwarz kann e4xd3 ep
    runTest(
        "En passant (black e4xd3 ep)",
        "8/8/8/8/3Pp3/8/8/4K3 b - d3 0 1",
        BLACK
    );

    return 0;
}

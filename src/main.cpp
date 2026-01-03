#include <iostream>
#include <string>
#include "../include/board.h"
#include "../include/movegen.h"
#include "../include/fen.h"
#include "../include/makemove.h"
#include "../include/figures/knights.h"
#include "../include/figures/king.h"
#include "../include/figures/sliding.h"
#include "../include/figures/pawns.h"
#include "../include/attacks.h"
#include "../include/perft.h"
#include "../include/search.h"
#include "../include/tt.h"

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

// ... deine bestehenden Helper-Funktionen bleiben ...

static void runKnightGenTest(const std::string& name, const std::string& fen, Color side, int expectedMoves) {
    Board b;
    if (!loadFEN(b, fen)) {
        std::cout << "[FAIL] " << name << ": FEN konnte nicht geladen werden\n";
        return;
    }

    auto moves = generateKnightMoves(b, side);

    std::cout << "\n=== " << name << " ===\n";
    std::cout << "FEN: " << fen << "\n";
    std::cout << "Knight moves: " << moves.size() << " (expected " << expectedMoves << ")\n";

    for (const auto& m : moves) {
        // wir nutzen deinen printMove, aber der druckt promo/cap; ist ok
        printMove(m);
    }

    if (static_cast<int>(moves.size()) != expectedMoves) {
        std::cout << "[FAIL] " << name << ": falsche Anzahl Springerzüge\n";
        return;
    }

    std::cout << "[OK] " << name << "\n";
}

static void runKnightRoundtripTest(const std::string& name, const std::string& fen, Color side, int expectedMoves) {
    Board b;
    if (!loadFEN(b, fen)) {
        std::cout << "[FAIL] " << name << ": FEN konnte nicht geladen werden\n";
        return;
    }

    auto moves = generateKnightMoves(b, side);

    std::cout << "\n=== " << name << " ===\n";
    std::cout << "FEN: " << fen << "\n";
    std::cout << "Knight moves: " << moves.size() << " (expected " << expectedMoves << ")\n";

    for (const auto& m : moves) {
        printMove(m);
    }

    if (static_cast<int>(moves.size()) != expectedMoves) {
        std::cout << "[FAIL] " << name << ": falsche Anzahl Springerzüge\n";
        return;
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

        // Knight darf kein EP-Target setzen
        if (b.enPassantTarget != 0ULL) {
            std::cout << "[FAIL] Knight-Move hat enPassantTarget gesetzt: ";
            printMove(m);
            return;
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

    std::cout << "[OK] Roundtrip+Checks bestanden für " << ok << " Knight-Moves.\n";
}

static void runKingGenTest(const std::string& name, const std::string& fen, Color side, int expectedMoves) {
    Board b;
    if (!loadFEN(b, fen)) {
        std::cout << "[FAIL] " << name << ": FEN konnte nicht geladen werden\n";
        return;
    }

    auto moves = generateKingMoves(b, side);

    std::cout << "\n=== " << name << " ===\n";
    std::cout << "FEN: " << fen << "\n";
    std::cout << "King moves: " << moves.size() << " (expected " << expectedMoves << ")\n";

    for (const auto& m : moves) {
        printMove(m);
    }

    if (static_cast<int>(moves.size()) != expectedMoves) {
        std::cout << "[FAIL] " << name << ": falsche Anzahl Königszüge\n";
        return;
    }

    std::cout << "[OK] " << name << "\n";
}

static void runKingRoundtripTest(const std::string& name, const std::string& fen, Color side, int expectedMoves) {
    Board b;
    if (!loadFEN(b, fen)) {
        std::cout << "[FAIL] " << name << ": FEN konnte nicht geladen werden\n";
        return;
    }

    auto moves = generateKingMoves(b, side);

    std::cout << "\n=== " << name << " ===\n";
    std::cout << "FEN: " << fen << "\n";
    std::cout << "King moves: " << moves.size() << " (expected " << expectedMoves << ")\n";

    for (const auto& m : moves) {
        printMove(m);
    }

    if (static_cast<int>(moves.size()) != expectedMoves) {
        std::cout << "[FAIL] " << name << ": falsche Anzahl Königszüge\n";
        return;
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

        // King darf kein EP-Target setzen
        if (b.enPassantTarget != 0ULL) {
            std::cout << "[FAIL] King-Move hat enPassantTarget gesetzt: ";
            printMove(m);
            return;
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

    std::cout << "[OK] Roundtrip+Checks bestanden für " << ok << " King-Moves.\n";
}

// Generic Helper für Sliding-Tests
static void runSlidingGenTest(const std::string& name,
                              const std::string& fen,
                              Color side,
                              int expectedMoves,
                              std::vector<Move> (*genFn)(const Board&, Color)) {
    Board b;
    if (!loadFEN(b, fen)) {
        std::cout << "[FAIL] " << name << ": FEN konnte nicht geladen werden\n";
        return;
    }

    auto moves = genFn(b, side);

    std::cout << "\n=== " << name << " ===\n";
    std::cout << "FEN: " << fen << "\n";
    std::cout << "Moves: " << moves.size() << " (expected " << expectedMoves << ")\n";

    for (const auto& m : moves) {
        printMove(m);
    }

    if (static_cast<int>(moves.size()) != expectedMoves) {
        std::cout << "[FAIL] " << name << ": falsche Anzahl Züge\n";
        return;
    }

    std::cout << "[OK] " << name << "\n";
}

static void runSlidingRoundtripTest(const std::string& name,
                                    const std::string& fen,
                                    Color side,
                                    int expectedMoves,
                                    std::vector<Move> (*genFn)(const Board&, Color)) {
    Board b;
    if (!loadFEN(b, fen)) {
        std::cout << "[FAIL] " << name << ": FEN konnte nicht geladen werden\n";
        return;
    }

    auto moves = genFn(b, side);

    std::cout << "\n=== " << name << " ===\n";
    std::cout << "FEN: " << fen << "\n";
    std::cout << "Moves: " << moves.size() << " (expected " << expectedMoves << ")\n";

    if (static_cast<int>(moves.size()) != expectedMoves) {
        std::cout << "[FAIL] " << name << ": falsche Anzahl Züge\n";
        return;
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

        // Sliding darf kein EP-Target setzen
        if (b.enPassantTarget != 0ULL) {
            std::cout << "[FAIL] Sliding-Move hat enPassantTarget gesetzt: ";
            printMove(m);
            return;
        }

        unmakeMove(b, m, u);

        if (!boardsEqual(b, original)) {
            std::cout << "[FAIL] Board nach unmakeMove ungleich original bei Move: ";
            printMove(m);
            return;
        }
        ok++;
    }

    std::cout << "[OK] Roundtrip+Checks bestanden für " << ok << " Moves.\n";
}

static void runLegalTest(const std::string& name,
                         const std::string& fen,
                         Color side,
                         int expectedLegalMoves) {
    Board b;
    if (!loadFEN(b, fen)) {
        std::cout << "[FAIL] " << name << ": FEN konnte nicht geladen werden\n";
        return;
    }

    auto pseudo = generatePseudoLegalMoves(b, side);
    auto legal  = generateLegalMoves(b, side);

    std::cout << "\n=== " << name << " ===\n";
    std::cout << "FEN: " << fen << "\n";
    std::cout << "Pseudo moves: " << pseudo.size() << "\n";
    std::cout << "Legal moves : " << legal.size() << " (expected " << expectedLegalMoves << ")\n";

    if (static_cast<int>(legal.size()) != expectedLegalMoves) {
        std::cout << "[FAIL] " << name << ": falsche Anzahl legaler Züge\n";
        std::cout << "Legal moves list:\n";
        for (const auto& m : legal) printMove(m);
        return;
    }

    std::cout << "[OK] " << name << "\n";
}

static uint64_t perftInternal(Board& board, int depth) {
    if (depth <= 0) return 1ULL;

    uint64_t nodes = 0ULL;
    Color side = board.sideToMove;
    auto moves = generateLegalMoves(board, side);

    for (const auto& m : moves) {
        UndoState u{};
        if (!makeMove(board, m, u)) continue;
        nodes += perftInternal(board, depth - 1);
        unmakeMove(board, m, u);
    }
    return nodes;
}

static void perftDivide(Board& board, int depth) {
    Color side = board.sideToMove;
    auto moves = generateLegalMoves(board, side);

    uint64_t total = 0ULL;
    for (const auto& m : moves) {
        UndoState u{};
        if (!makeMove(board, m, u)) continue;

        uint64_t n = perftInternal(board, depth - 1);
        unmakeMove(board, m, u);

        total += n;

        // einfache Notation: from-to + Flags
        std::cout << squareToString(m.from) << squareToString(m.to);
        if (m.flags & PROMOTION) std::cout << pieceToChar(m.promotion);
        if (m.flags & CASTLING) std::cout << " (O-O/O-O-O)";
        std::cout << ": " << n << "\n";
    }
    std::cout << "DIVIDE TOTAL: " << total << "\n";
}

int main() {
    initZobrist();
    // Deine Pawn-Tests wie gehabt:
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

    runTest(
        "Normal capture (white e4xf5)",
        "8/8/8/5p2/4P3/8/8/4k3 w - - 0 1",
        WHITE
    );

    runTest(
        "Double push (white e2->e4 sets EP square e3)",
        "8/8/8/8/8/8/4P3/4k3 w - - 0 1",
        WHITE
    );

    runTest(
        "Promotion (black pawn a2->a1)",
        "8/8/8/8/8/8/p7/4K3 b - - 0 1",
        BLACK
    );

    runTest(
        "En passant (black e4xd3 ep)",
        "8/8/8/8/3Pp3/8/8/4K3 b - d3 0 1",
        BLACK
    );

    // Neuer Springer-Test: weißer Springer auf d4, ansonsten leeres Brett + ein schwarzer König
    // d4 hat in leerer Stellung genau 8 Springerzüge: b3 b5 c2 c6 e2 e6 f3 f5
    runKnightGenTest(
        "Knight gen (white knight on d4, empty board)",
        "8/8/8/8/3N4/8/8/4k3 w - - 0 1",
        WHITE,
        8
    );

    // Optional: Corner-Test (a1 hat nur 2 Züge: b3, c2)
    runKnightGenTest(
        "Knight gen (white knight on a1)",
        "8/8/8/8/8/8/8/N3k3 w - - 0 1",
        WHITE,
        2
    );

    runKnightRoundtripTest(
        "Knight roundtrip (white knight on d4, empty board)",
        "8/8/8/8/3N4/8/8/4k3 w - - 0 1",
        WHITE,
        8
    );

    runKnightRoundtripTest(
        "Knight roundtrip (white knight on a1)",
        "8/8/8/8/8/8/8/N3k3 w - - 0 1",
        WHITE,
        2
    );

    runKnightRoundtripTest(
        "Knight roundtrip (white knight captures on f5)",
        "8/8/8/5p2/3N4/8/8/4k3 w - - 0 1",
        WHITE,
        8
    );

    runKingGenTest(
        "King gen (white king on d4, empty board)",
        "8/8/8/8/3K4/8/8/4k3 w - - 0 1",
        WHITE,
        8
    );

    runKingGenTest(
        "King gen (white king on a1)",
        "8/8/8/8/8/8/8/K3k3 w - - 0 1",
        WHITE,
        3
    );

    runKingRoundtripTest(
        "King roundtrip (white king on d4, empty board)",
        "8/8/8/8/3K4/8/8/4k3 w - - 0 1",
        WHITE,
        8
    );

    runKingRoundtripTest(
        "King roundtrip (white king on a1)",
        "8/8/8/8/8/8/8/K3k3 w - - 0 1",
        WHITE,
        3
    );

    // In main() am Ende hinzufügen:
    runSlidingGenTest(
        "Bishop gen (white bishop on d4, empty board)",
        "8/8/8/8/3B4/8/8/4k3 w - - 0 1",
        WHITE,
        13,
        generateBishopMoves
    );

    runSlidingGenTest(
        "Rook gen (white rook on d4, empty board)",
        "8/8/8/8/3R4/8/8/4k3 w - - 0 1",
        WHITE,
        14,
        generateRookMoves
    );

    runSlidingGenTest(
        "Queen gen (white queen on d4, empty board)",
        "8/8/8/8/3Q4/8/8/4k3 w - - 0 1",
        WHITE,
        27,
        generateQueenMoves
    );

    runSlidingGenTest(
        "Bishop gen (capture stops ray: bishop d4, enemy f6)",
        "8/8/5p2/8/3B4/8/8/4k3 w - - 0 1",
        WHITE,
        11,
        generateBishopMoves
    );

    runSlidingGenTest(
        "Rook gen (capture stops ray: rook d4, enemy d6)",
        "8/8/3p4/8/3R4/8/8/4k3 w - - 0 1",
        WHITE,
        12,
        generateRookMoves
    );

    runSlidingRoundtripTest(
        "Bishop roundtrip (white bishop on d4, empty board)",
        "8/8/8/8/3B4/8/8/4k3 w - - 0 1",
        WHITE,
        13,
        generateBishopMoves
    );

    runSlidingRoundtripTest(
        "Rook roundtrip (white rook on d4, empty board)",
        "8/8/8/8/3R4/8/8/4k3 w - - 0 1",
        WHITE,
        14,
        generateRookMoves
    );

    runSlidingRoundtripTest(
        "Queen roundtrip (white queen on d4, empty board)",
        "8/8/8/8/3Q4/8/8/4k3 w - - 0 1",
        WHITE,
        27,
        generateQueenMoves
    );

    runLegalTest(
        "Legal filter: King may not move into check (rook on e3)",
        "4k3/8/8/8/8/4r3/8/4K3 w - - 0 1",
        WHITE,
        4
    );

    runLegalTest(
        "Legal filter: pinned knight cannot move (rook on e8 pins N on e2)",
        "4r3/8/8/8/8/8/4N3/4K3 w - - 0 1",
        WHITE,
        4
    );

    // --- Castling Tests ---

    // 1) Beide Rochaden für Weiß müssen legal sein:
    // Brett: nur Könige + weiße Türme auf a1/h1, schwarzer König auf e8
    // Felder zwischen König und Türmen sind frei, keine Angriffe auf e1/f1/g1 oder e1/d1/c1.
    // Erwartung: legal moves = 26
    runLegalTest(
        "Castling: white can castle both sides",
        "4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1",
        WHITE,
        26
    );

    // 2) Kingside-Rochade ist verboten, wenn f1 angegriffen ist:
    // Schwarzer Läufer auf c4 greift f1 an (c4->d3->e2->f1).
    // Queenside sollte weiterhin möglich sein.
    // Erwartung: legal moves = 25 (wie oben, aber -1 wegen fehlender O-O)
    runLegalTest(
        "Castling: white kingside forbidden if f1 is attacked (bishop c4)",
        "4k3/8/8/8/2b5/8/8/R3K2R w KQ - 0 1",
        WHITE,
        23
    );

    // 3) Keine Rochade, wenn der König aktuell im Schach steht:
    // Schwarzer Turm auf e3 gibt Schach auf e1.
    // Erwartung: keine Rochadezüge, legal wie im früheren Test = 4
    runLegalTest(
        "Castling: forbidden while in check (rook on e3)",
        "4k3/8/8/8/8/4r3/8/R3K2R w KQ - 0 1",
        WHITE,
        4
    );

    {
        Board start;
        initBitboards(start);

        std::cout << "\n=== PERFT DIVIDE Startposition depth 3 ===\n";
        perftDivide(start, 3);
    }

    {
        Board start;
        initBitboards(start);

        int depth = 3;

        std::cout << "\n=== TOP MOVES Startposition ===\n";
        printTopMoves(start, depth, 5);

        // Move bm = findBestMove(start, depth);
        Move bm = findBestMoveIterative(start, depth);
        std::cout << "Bestmove: " << squareToString(bm.from) << squareToString(bm.to) << "\n";
    }

    {
        Board start;
        initBitboards(start);

        int depth = 3;
        Move bm = findBestMove(start, depth);

        std::cout << "\n=== SEARCH Startposition ===\n";
        std::cout << "Depth: " << depth << "\n";
        std::cout << "Bestmove: " << squareToString(bm.from) << squareToString(bm.to) << "\n";
    }

    {
        Board b;
        const std::string fen = "3rk3/8/8/6b1/8/8/8/3QK3 w - - 0 1";
        if (!loadFEN(b, fen)) {
            std::cout << "[FAIL] konnte Test-FEN nicht laden\n";
        } else {
            std::cout << "\n=== QUIESCENCE TEST: Poisoned Rook ===\n";
            std::cout << "FEN: " << fen << "\n";

            int depth = 1; // gerade hier zeigt Quiescence oft den Unterschied
            Move bm = findBestMove(b, depth);

            std::cout << "Depth: " << depth << "\n";
            std::cout << "Bestmove: " << squareToString(bm.from) << squareToString(bm.to) << "\n";
            std::cout << "Hinweis: Qxd8 (d1d8) wäre hier ein \"Poisoned\" Capture, weil ...Bxd8 folgt.\n";
        }
    }
    return 0;
}

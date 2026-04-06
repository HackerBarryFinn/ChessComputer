#include "test_runner.h"

#include "chess/core/board.h"
#include "chess/core/movegen.h"
#include "chess/rules/fen.h"
#include "chess/rules/makemove.h"
#include "chess/rules/perft.h"
#include "chess/search/search.h"
#include "chess/search/tt.h"
#include "chess/pieces/knights.h"
#include "chess/pieces/sliding.h"

#include <iostream>
#include <string>
#include <vector>
#include <chrono>

// -------------------- Helpers --------------------

static std::string sqToString(int sq) {
    int file = sq % 8;
    int rank = sq / 8;
    std::string s;
    s += static_cast<char>('a' + file);
    s += static_cast<char>('1' + rank);
    return s;
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

static void printMove(const Move& m) {
    std::cout << sqToString(m.from) << sqToString(m.to)
              << " flags=" << flagsToString(m.flags);
    if (m.flags & PROMOTION) std::cout << " promo=" << pieceToChar(m.promotion);
    if (m.flags & CAPTURE)   std::cout << " cap=" << pieceToChar(m.captured);
    std::cout << "\n";
}

static bool boardsEqual(const Board& a, const Board& b) {
    if (a.bitboards != b.bitboards) return false;
    if (a.occupied[WHITE] != b.occupied[WHITE]) return false;
    if (a.occupied[BLACK] != b.occupied[BLACK]) return false;
    if (a.allOccupied != b.allOccupied) return false;
    if (a.kingSq[WHITE] != b.kingSq[WHITE] || a.kingSq[BLACK] != b.kingSq[BLACK]) return false;
    if (a.sideToMove != b.sideToMove) return false;
    if (a.enPassantTarget != b.enPassantTarget) return false;
    if (a.whiteKingsideCastle != b.whiteKingsideCastle) return false;
    if (a.whiteQueensideCastle != b.whiteQueensideCastle) return false;
    if (a.blackKingsideCastle != b.blackKingsideCastle) return false;
    if (a.blackQueensideCastle != b.blackQueensideCastle) return false;
    return true;
}

static bool expect(bool cond, const std::string& msg) {
    if (!cond) {
        std::cout << "[FAIL] " << msg << "\n";
        return false;
    }
    return true;
}

// -------------------- Testgruppen --------------------

static bool testMakeUnmakeBasics() {
    std::cout << "\n=== testMakeUnmakeBasics ===\n";

    // Beispiel 1: Promotion (weiß)
    {
        Board b;
        if (!loadFEN(b, "8/P7/8/8/8/8/8/4k3 w - - 0 1"))
            return expect(false, "FEN load (promotion)");

        auto moves = generateLegalMoves(b, b.sideToMove);
        bool foundPromo = false;

        for (const auto& m : moves) {
            if ((m.flags & PROMOTION) == 0) continue;
            foundPromo = true;

            Board orig = b;
            UndoState u{};
            if (!makeMove(b, m, u)) return expect(false, "makeMove promotion failed");
            unmakeMove(b, m, u);

            if (!boardsEqual(b, orig)) return expect(false, "board mismatch after promotion unmake");
            break;
        }
        if (!foundPromo) return expect(false, "no promotion move found");
    }

    // Beispiel 2: En Passant (weiß e5xd6 ep)
    {
        Board b;
        if (!loadFEN(b, "8/8/8/3pP3/8/8/8/4k3 w - d6 0 1"))
            return expect(false, "FEN load (ep)");

        auto moves = generateLegalMoves(b, b.sideToMove);
        bool foundEp = false;

        for (const auto& m : moves) {
            if ((m.flags & EN_PASSANT) == 0) continue;
            foundEp = true;

            Board orig = b;
            UndoState u{};
            if (!makeMove(b, m, u)) return expect(false, "makeMove ep failed");
            unmakeMove(b, m, u);

            if (!boardsEqual(b, orig)) return expect(false, "board mismatch after ep unmake");
            break;
        }
        if (!foundEp) return expect(false, "no en-passant move found");
    }

    std::cout << "[OK] testMakeUnmakeBasics\n";
    return true;
}

static bool testMovegenBasics() {
    std::cout << "\n=== testMovegenBasics ===\n";

    // Beispiel 1: Knight moves in leerer Stellung (d4 -> 8)
    {
        Board b;
        if (!loadFEN(b, "8/8/8/8/3N4/8/8/4k3 w - - 0 1"))
            return expect(false, "FEN load (knight)");

        MoveList moves;
        generateKnightMoves(b, WHITE, moves);
        if (!expect(moves.size == 8, "knight moves expected 8")) return false;
    }

    // Beispiel 2: Rook moves in leerer Stellung (d4 -> 14)
    {
        Board b;
        if (!loadFEN(b, "8/8/8/8/3R4/8/8/4k3 w - - 0 1"))
            return expect(false, "FEN load (rook)");

        MoveList moves;
        generateRookMoves(b, WHITE, moves);
        if (!expect(moves.size == 14, "rook moves expected 14")) return false;
    }

    std::cout << "[OK] testMovegenBasics\n";
    return true;
}

static bool testLegalBasics() {
    std::cout << "\n=== testLegalBasics ===\n";

    // Beispiel 1: König darf nicht ins Schach laufen (schwarzer Turm e3)
    {
        Board b;
        if (!loadFEN(b, "4k3/8/8/8/8/4r3/8/4K3 w - - 0 1"))
            return expect(false, "FEN load (king into check)");

        auto legal = generateLegalMoves(b, WHITE);
        if (!expect(static_cast<int>(legal.size()) == 4, "legal moves expected 4 in check-avoid test")) return false;
    }

    // Beispiel 2: Castling erlaubt (beide Seiten)
    {
        Board b;
        if (!loadFEN(b, "4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1"))
            return expect(false, "FEN load (castling)");

        auto legal = generateLegalMoves(b, WHITE);

        bool hasO_O = false;
        bool hasO_O_O = false;
        for (const auto& m : legal) {
            if ((m.flags & CASTLING) == 0) continue;
            if (m.to == 6) hasO_O = true;   // e1->g1
            if (m.to == 2) hasO_O_O = true; // e1->c1
        }

        if (!expect(hasO_O && hasO_O_O, "expected both castlings available")) return false;
    }

    std::cout << "[OK] testLegalBasics\n";
    return true;
}

static bool testPerftStartpos() {
    std::cout << "\n=== testPerftStartpos ===\n";
    Board b;
    if (!loadFEN(b, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"))
        return expect(false, "FEN load (startpos)");

    struct Case { int depth; uint64_t expected; };
    const Case cases[] = {
        {1, 20ULL},
        {2, 400ULL},
        {3, 8902ULL},
        {4, 197281ULL},
        {5, 4865609ULL},
        {6, 119060324ULL},
    };

    for (const auto& c : cases) {
        Board tmp = b;
        auto t0 = std::chrono::steady_clock::now();
        uint64_t got = perft(tmp, c.depth);
        auto t1 = std::chrono::steady_clock::now();

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        std::cout << "perft(" << c.depth << ") = " << got << " (exp " << c.expected << ") [" << ms << " ms]\n";

        if (!expect(got == c.expected, "perft mismatch at depth " + std::to_string(c.depth))) return false;
    }

    std::cout << "[OK] testPerftStartpos\n";
    return true;
}

static bool testSearchSmoke() {
    std::cout << "\n=== testSearchSmoke ===\n";

    Board b;
    initBitboards(b); // Startpos

    // nur Smoke-Test: soll laufen und einen halbwegs normalen Zug zurückgeben
    constexpr int depth = 3;
    Move bm = findBestMoveIterative(b, depth);

    std::cout << "bestmove @depth " << depth << ": " << sqToString(bm.from) << sqToString(bm.to) << "\n";

    // sehr weiche Plausibilitätsprüfung
    if (!expect(bm.from >= 0 && bm.from < 64 && bm.to >= 0 && bm.to < 64, "bestmove squares in range")) return false;

    std::cout << "[OK] testSearchSmoke\n";
    return true;
}

// -------------------- Entry --------------------

int runAllTests() {
    initZobrist();

    bool ok = true;
    ok = testMakeUnmakeBasics() && ok;
    ok = testMovegenBasics() && ok;
    ok = testLegalBasics() && ok;
    ok = testPerftStartpos() && ok;
    ok = testSearchSmoke() && ok;

    std::cout << "\n=== RESULT: " << (ok ? "OK" : "FAIL") << " ===\n";
    return ok ? 0 : 1;
}
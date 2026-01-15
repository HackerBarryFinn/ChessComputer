#include "app/game.h"

#include "chess/core/utils.h"
#include "chess/rules/fen.h"

#include <cctype>

Game::Game() {
    resetToStartpos();
}

void Game::resetToStartpos() {
    board_ = Board{};
    initBitboards(board_);
    history_.clear();
}

bool Game::loadFromFEN(const std::string& fen) {
    Board tmp{};
    if (!loadFEN(tmp, fen)) return false;
    board_ = tmp;
    history_.clear();
    return true;
}

bool Game::applyMove(const Move& m) {
    UndoState u{};
    if (!makeMove(board_, m, u)) return false;
    history_.push_back(HistoryEntry{m, u});
    return true;
}

bool Game::undo() {
    if (history_.empty()) return false;
    const HistoryEntry last = history_.back();
    history_.pop_back();
    unmakeMove(board_, last.move, last.undo);
    return true;
}

int Game::squareFromString(const std::string& s, int offset) {
    if (offset + 1 >= static_cast<int>(s.size())) return -1;
    const char file = s[offset + 0];
    const char rank = s[offset + 1];
    if (file < 'a' || file > 'h') return -1;
    if (rank < '1' || rank > '8') return -1;
    const int f = file - 'a';
    const int r = rank - '1';
    return r * 8 + f;
}

int Game::promoFromChar(char c) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    switch (c) {
        case 'q': return QUEEN;
        case 'r': return ROOK;
        case 'b': return BISHOP;
        case 'n': return KNIGHT;
        default:  return -1;
    }
}

bool Game::parseUci(const std::string& uci, int& fromSq, int& toSq, int& promoPtOrMinus1) {
    promoPtOrMinus1 = -1;
    if (uci.size() != 4 && uci.size() != 5) return false;

    fromSq = squareFromString(uci, 0);
    toSq   = squareFromString(uci, 2);
    if (fromSq < 0 || toSq < 0) return false;

    if (uci.size() == 5) {
        promoPtOrMinus1 = promoFromChar(uci[4]);
        if (promoPtOrMinus1 == -1) return false;
    }
    return true;
}

bool Game::sameMove(const Move& legal, int fromSq, int toSq, int promoPtOrMinus1) {
    if (legal.from != fromSq) return false;
    if (legal.to != toSq) return false;

    const bool wantsPromo = (promoPtOrMinus1 != -1);
    const bool isPromoMove = (legal.flags & PROMOTION) != 0;

    if (wantsPromo != isPromoMove) return false;

    if (wantsPromo) {
        return legal.promotion == promoPtOrMinus1;
    }
    return true;
}

bool Game::applyUci(const std::string& uci) {
    int fromSq = -1, toSq = -1, promo = -1;
    if (!parseUci(uci, fromSq, toSq, promo)) return false;

    MoveList moves;
    generateLegalMoves(board_, board_.sideToMove, moves);

    for (int i = 0; i < moves.size; ++i) {
        const Move& m = moves[i];
        if (!sameMove(m, fromSq, toSq, promo)) continue;
        return applyMove(m); // den "echten" legal Move anwenden
    }
    return false;
}

std::string Game::moveToUci(const Move& m) {
    std::string s;
    s += squareToString(m.from);
    s += squareToString(m.to);

    if (m.flags & PROMOTION) {
        // UCI nimmt kleinbuchstaben
        char pc = pieceToChar(m.promotion);
        pc = static_cast<char>(std::tolower(static_cast<unsigned char>(pc)));
        s += pc;
    }
    return s;
}
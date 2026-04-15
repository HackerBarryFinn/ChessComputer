#pragma once

#include "chess/core/board.h"
#include "chess/core/movegen.h"
#include "chess/rules/makemove.h"

#include <string>
#include <vector>

class Game {
public:
    Game();

    // Startposition
    void resetToStartpos();

    // FEN laden
    bool loadFromFEN(const std::string& fen);

    [[nodiscard]] const Board& board() const { return board_; }

    // Anwenden eines bereits legalen Move-Objekts
    bool applyMove(const Move& m);

    // Anwenden eines Zugs
    bool applyUci(const std::string& uci);

    // Undo letzter Zug
    bool undo();

    // Zugriff auf die Historie (für GUI später)
    struct HistoryEntry {
        Move move{};
        UndoState undo{};
    };
    [[nodiscard]] const std::vector<HistoryEntry>& history() const { return history_; }

    // Hilfsfunktionen
    static std::string moveToUci(const Move& m);

private:
    Board board_{};
    std::vector<HistoryEntry> history_{};

    static bool parseUci(const std::string& uci, int& fromSq, int& toSq, int& promoPtOrMinus1);
    static int squareFromString(const std::string& s, int offset);
    static int promoFromChar(char c);

    static bool sameMove(const Move& legal, int fromSq, int toSq, int promoPtOrMinus1);
};
#include "app/game.h"

#include "chess/core/board.h"
#include "chess/search/search.h"

#include <iostream>
#include <string>

int main() {
    Game game;
    constexpr int ENGINE_DEPTH = 7;

    std::cout << "ChessComputer (CLI)\n"
                 "Eingabe: UCI-Zug wie e2e4 oder e7e8q\n"
                 "Commands: quit | undo | startpos\n\n";

    while (true) {
        printBoard(game.board());

        if (game.board().sideToMove == WHITE) {
            std::cout << "Du (Weiss) > ";
            std::string line;
            if (!std::getline(std::cin, line)) break;

            if (line == "quit") break;
            if (line == "undo") { game.undo(); continue; }
            if (line == "startpos") { game.resetToStartpos(); continue; }

            if (!game.applyUci(line)) {
                std::cout << "Ungueltiger/illegaler Zug.\n";
            }
        } else {
            std::cout << "Engine denkt (Schwarz, depth " << ENGINE_DEPTH << ")...\n";

            // Suche arbeitet per make/unmake auf einem nicht-const Board&.
            Board tmp = game.board();
            Move best = findBestMove(tmp, ENGINE_DEPTH);

            const std::string uci = Game::moveToUci(best);
            std::cout << "Engine spielt: " << uci << "\n";

            if (!game.applyUci(uci)) {
                std::cout << "Interner Fehler: Engine-Zug konnte nicht angewendet werden.\n";
                break;
            }
        }
    }

    return 0;
}
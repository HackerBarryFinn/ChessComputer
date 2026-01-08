#pragma once

#include <vector>
#include "../core/movegen.h"
#include "../core/board.h"

enum class TTFlag : uint8_t { EXACT = 0, LOWERBOUND = 1, UPPERBOUND = 2 };

struct TTEntry {
    uint64_t key = 0;
    int depth = -1;
    int score = 0;
    TTFlag flag = TTFlag::EXACT;
    Move bestMove{};
};

class TranspositionTable {
public:
    explicit TranspositionTable(size_t sizePow2 = (1u << 20)); // 1,048,576 Einträge
    void clear();

    bool probe(uint64_t key, TTEntry& out) const;
    void store(uint64_t key, int depth, int score, TTFlag flag, const Move& bestMove);

private:
    size_t mask_;
    std::vector<TTEntry> table_;
};

// Zobrist Hash: berechnet Hash für komplette Stellung (Board-State).
uint64_t computeZobrist(const Board& board);

// Muss 1x beim Programmstart aufgerufen werden (Keys initialisieren).
void initZobrist();
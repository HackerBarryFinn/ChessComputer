#include "chess/search/tt.h"
#include <random>
#include <bit>

namespace {
    uint64_t ZPieces[2][6][64];
    uint64_t ZSide;
    uint64_t ZCastle[16];      // 4 bits: WK WQ BK BQ
    uint64_t ZEnPassant[9];    // 0..7 file, 8 = none
    bool zobristReady = false;

    uint64_t rand64(std::mt19937_64& rng) {
        std::uniform_int_distribution<uint64_t> dist;
        return dist(rng);
    }

    int castleIndex(const Board& b) {
        int idx = 0;
        if (b.whiteKingsideCastle)  idx |= 1;
        if (b.whiteQueensideCastle) idx |= 2;
        if (b.blackKingsideCastle)  idx |= 4;
        if (b.blackQueensideCastle) idx |= 8;
        return idx;
    }

    int enPassantFileIndex(uint64_t epTarget) {
        if (epTarget == 0ULL) return 8; // none
        int sq = std::countr_zero(epTarget);
        int file = sq % 8;
        return file; // 0..7
    }
}

void initZobrist() {
    std::mt19937_64 rng(0xC0FFEE123456789ULL);

    for (auto & ZPiece : ZPieces) {
        for (auto & pt : ZPiece) {
            for (unsigned long long & sq : pt) {
                sq = rand64(rng);
            }
        }
    }

    ZSide = rand64(rng);

    for (unsigned long long & i : ZCastle) i = rand64(rng);
    for (unsigned long long & i : ZEnPassant) i = rand64(rng);

    zobristReady = true;
}

uint64_t computeZobrist(const Board& board) {
    if (!zobristReady) initZobrist();

    uint64_t key = 0ULL;

    for (int c = 0; c < 2; ++c) {
        for (int pt = 0; pt < 6; ++pt) {
            uint64_t bb = board.bitboards[c][pt];
            while (bb) {
                int sq = std::countr_zero(bb);
                bb &= (bb - 1);
                key ^= ZPieces[c][pt][sq];
            }
        }
    }

    if (board.sideToMove == BLACK) key ^= ZSide;

    key ^= ZCastle[castleIndex(board)];
    key ^= ZEnPassant[enPassantFileIndex(board.enPassantTarget)];

    return key;
}

// ---- TT ----

TranspositionTable::TranspositionTable(size_t sizePow2)
    : mask_(sizePow2 - 1), table_(sizePow2) {}

void TranspositionTable::clear() {
    std::ranges::fill(table_, TTEntry{});
}

bool TranspositionTable::probe(uint64_t key, TTEntry& out) const {
    const TTEntry& e = table_[static_cast<size_t>(key) & mask_];
    if (e.key == key && e.depth >= 0) {
        out = e;
        return true;
    }
    return false;
}

void TranspositionTable::store(uint64_t key, int depth, int score, TTFlag flag, const Move& bestMove) {
    TTEntry& e = table_[static_cast<size_t>(key) & mask_];
    // simple replacement: depth bevorzugen
    if (e.depth <= depth || e.key != key) {
        e.key = key;
        e.depth = depth;
        e.score = score;
        e.flag = flag;
        e.bestMove = bestMove;
    }
}
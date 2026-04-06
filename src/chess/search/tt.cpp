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

/**
 * @brief Initializes Zobrist hashing values used for chess position hashing.
 *
 * This function generates pseudo-random values for Zobrist hashing of chess positions.
 * It initializes arrays for pieces, castling rights, en passant target squares,
 * and the side to move. The values are derived using a 64-bit random number generator
 * and stored in the following variables:
 * - ZPieces: A 3D array storing random values for pieces on specific squares.
 * - ZCastle: An array representing castling rights (White King, White Queen, Black King, Black Queen).
 * - ZEnPassant: An array representing en passant target squares.
 * - ZSide: A random value to differentiate between sides to move.
 *
 * The function ensures the zobristReady flag is set to true upon completion to mark
 * that the Zobrist hashing initialization is complete.
 */
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

/**
 * @brief Computes the Zobrist hash key for the given chess board position.
 *
 * This function calculates a unique 64-bit hash value representing a chess board position.
 * The Zobrist hashing method involves XOR-ing pseudo-random values associated with the
 * position of pieces on the board, castling rights, en passant target squares, and
 * the side to move. It ensures accurate representation of a board state for use in
 * transposition tables or other hashing use cases.
 *
 * If Zobrist hashing values are not yet initialized, the function calls `initZobrist()`
 * to generate them before proceeding with the hash computation.
 *
 * @param board The current chess board state containing information about piece positions,
 *              side to move, castling rights, and en passant target.
 * @return A 64-bit unsigned integer representing the Zobrist hash key of the chess board.
 */
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

TranspositionTable::TranspositionTable(size_t sizePow2)
    : mask_(sizePow2 - 1), table_(sizePow2) {}

/**
 * @brief Clears the transposition table by resetting all entries to their default values.
 *
 * This function iterates over the internal storage of the transposition table and fills
 * it with default-constructed entries of type `TTEntry`. As a result, all previously
 * stored positions, scores, depths, and associated data are erased. This is typically
 * used to reset the transposition table, ensuring it no longer contains stale or irrelevant data.
 */
void TranspositionTable::clear() {
    std::ranges::fill(table_, TTEntry{});
}

/**
 * @brief Attempts to retrieve a transposition table entry for the given position key.
 *
 * This function probes the transposition table for an entry corresponding to the specified
 * Zobrist hash key. If a matching entry is found, with a valid depth (non-negative),
 * it copies the entry to the provided output parameter and returns true. Otherwise, it returns false.
 *
 * @param key The Zobrist hash key representing the chess position to probe.
 * @param out A reference to a TTEntry object where the found entry will be stored, if available.
 * @return True if a matching entry is found and valid, otherwise false.
 */
bool TranspositionTable::probe(uint64_t key, TTEntry& out) const {
    const TTEntry& e = table_[static_cast<size_t>(key) & mask_];
    if (e.key == key && e.depth >= 0) {
        out = e;
        return true;
    }
    return false;
}

/**
 * @brief Stores an entry into the transposition table.
 *
 * This function inserts an entry into the transposition table with the given parameters.
 * It employs a simple replacement strategy, favoring entries with greater depth
 * or replacing entries with a different hash key. The transposition table helps
 * reuse previously computed position evaluations, improving the efficiency
 * of the search in a chess engine.
 *
 * @param key The Zobrist hash key corresponding to a unique board position.
 * @param depth The search depth related to the stored entry.
 * @param score The evaluation score of the position.
 * @param flag The type of transposition table entry, indicating its bounds (EXACT, LOWERBOUND, UPPERBOUND).
 * @param bestMove The best move identified for the given position.
 */
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
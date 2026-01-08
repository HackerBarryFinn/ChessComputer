#include "../../include/figures/pawns.h"
#include "../../include/utils.h"

static inline uint64_t sqBB(int sq) { return 1ULL << sq; }

void generatePawnMoves(const Board &board, Color side, MoveList& out) {
    uint64_t pawns = board.bitboards[side][PAWN];

    constexpr uint64_t FILE_A = 0x0101010101010101ULL;
    constexpr uint64_t FILE_H = 0x8080808080808080ULL;
    constexpr uint64_t RANK_1 = 0x00000000000000FFULL;
    constexpr uint64_t RANK_8 = 0xFF00000000000000ULL;

    const uint64_t promoRank = (side == WHITE) ? RANK_8 : RANK_1;

    auto pushPromotionMoves = [&](int from, int to, bool isCapture) {
        const uint8_t baseFlags = static_cast<uint8_t>(PROMOTION | (isCapture ? CAPTURE : QUIET));
        const PieceType promoPieces[4] = {QUEEN, ROOK, BISHOP, KNIGHT};

        for (PieceType p : promoPieces) {
            Move m;
            m.from = from;
            m.to = to;
            m.moved = PAWN;
            m.promotion = static_cast<int>(p);
            m.flags = baseFlags;
            out.push(m);
        }
    };

    int shift = (side == WHITE) ? 8 : -8;

    uint64_t singlePushes = (side == WHITE)
                                ? (pawns << 8) & ~board.allOccupied
                                : (pawns >> 8) & ~board.allOccupied;

    uint64_t temp = singlePushes;
    while (temp) {
        int to = bitScanForward(temp);
        temp &= temp - 1;
        int from = to - shift;

        const bool isPromotion = (sqBB(to) & promoRank) != 0ULL;

        if (isPromotion) {
            pushPromotionMoves(from, to, false);
        } else {
            Move m;
            m.from = from;
            m.to = to;
            m.moved = PAWN;
            m.flags = QUIET;
            out.push(m);
        }
    }

    if (side == WHITE) {
        uint64_t rank2 = 0x000000000000FF00ULL;
        uint64_t doublePushes = ((pawns & rank2) << 16)
                                & ~board.allOccupied
                                & ~(board.allOccupied << 8);
        temp = doublePushes;
        while (temp) {
            int to = bitScanForward(temp);
            temp &= temp - 1;
            int from = to - 16;

            Move m;
            m.from = from;
            m.to = to;
            m.moved = PAWN;
            m.flags = DOUBLE_PUSH;
            out.push(m);
        }
    } else {
        uint64_t rank7 = 0x00FF000000000000ULL;
        uint64_t doublePushes = ((pawns & rank7) >> 16)
                                & ~board.allOccupied
                                & ~(board.allOccupied >> 8);
        temp = doublePushes;
        while (temp) {
            int to = bitScanForward(temp);
            temp &= temp - 1;
            int from = to + 16;

            Move m;
            m.from = from;
            m.to = to;
            m.moved = PAWN;
            m.flags = DOUBLE_PUSH;
            out.push(m);
        }
    }

    uint64_t enemyPieces = (side == WHITE) ? board.occupied[BLACK] : board.occupied[WHITE];

    uint64_t leftCaptures = (side == WHITE)
                                ? ((pawns & ~FILE_A) << 7) & enemyPieces
                                : ((pawns & ~FILE_A) >> 9) & enemyPieces;

    uint64_t rightCaptures = (side == WHITE)
                                 ? ((pawns & ~FILE_H) << 9) & enemyPieces
                                 : ((pawns & ~FILE_H) >> 7) & enemyPieces;

    temp = leftCaptures;
    while (temp) {
        int to = bitScanForward(temp);
        temp &= temp - 1;
        int from = (side == WHITE) ? to - 7 : to + 9;

        const bool isPromotion = (sqBB(to) & promoRank) != 0ULL;
        if (isPromotion) {
            pushPromotionMoves(from, to, true);
        } else {
            Move m;
            m.from = from;
            m.to = to;
            m.moved = PAWN;
            m.flags = CAPTURE;
            out.push(m);
        }
    }

    temp = rightCaptures;
    while (temp) {
        int to = bitScanForward(temp);
        temp &= temp - 1;
        int from = (side == WHITE) ? to - 9 : to + 7;

        const bool isPromotion = (sqBB(to) & promoRank) != 0ULL;
        if (isPromotion) {
            pushPromotionMoves(from, to, true);
        } else {
            Move m;
            m.from = from;
            m.to = to;
            m.moved = PAWN;
            m.flags = CAPTURE;
            out.push(m);
        }
    }

    if (board.enPassantTarget != 0ULL) {
        uint64_t ep = board.enPassantTarget;

        if (side == WHITE) {
            uint64_t epLeft  = ((pawns & ~FILE_A) << 7) & ep;
            uint64_t epRight = ((pawns & ~FILE_H) << 9) & ep;

            uint64_t t = epLeft;
            while (t) {
                int to = bitScanForward(t);
                t &= t - 1;
                int from = to - 7;

                Move m;
                m.from = from;
                m.to = to;
                m.moved = PAWN;
                m.captured = static_cast<int>(PAWN);
                m.flags = static_cast<uint8_t>(CAPTURE | EN_PASSANT);
                out.push(m);
            }

            t = epRight;
            while (t) {
                int to = bitScanForward(t);
                t &= t - 1;
                int from = to - 9;

                Move m;
                m.from = from;
                m.to = to;
                m.moved = PAWN;
                m.captured = static_cast<int>(PAWN);
                m.flags = static_cast<uint8_t>(CAPTURE | EN_PASSANT);
                out.push(m);
            }
        } else {
            uint64_t epLeft  = ((pawns & ~FILE_A) >> 9) & ep;
            uint64_t epRight = ((pawns & ~FILE_H) >> 7) & ep;

            uint64_t t = epLeft;
            while (t) {
                int to = bitScanForward(t);
                t &= t - 1;
                int from = to + 9;

                Move m;
                m.from = from;
                m.to = to;
                m.moved = PAWN;
                m.captured = static_cast<int>(PAWN);
                m.flags = static_cast<uint8_t>(CAPTURE | EN_PASSANT);
                out.push(m);
            }

            t = epRight;
            while (t) {
                int to = bitScanForward(t);
                t &= t - 1;
                int from = to + 7;

                Move m;
                m.from = from;
                m.to = to;
                m.moved = PAWN;
                m.captured = static_cast<int>(PAWN);
                m.flags = static_cast<uint8_t>(CAPTURE | EN_PASSANT);
                out.push(m);
            }
        }
    }
}

std::vector<Move> generatePawnMoves(const Board& board, Color side) {
    MoveList tmp;
    generatePawnMoves(board, side, tmp);
    return std::vector<Move>(tmp.data.begin(), tmp.data.begin() + tmp.size);
}
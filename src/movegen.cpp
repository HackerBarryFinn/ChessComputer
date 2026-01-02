#include "movegen.h"
#include "utils.h"   // für bitScanForward

std::vector<Move> generatePawnMoves(const Board &board, Color side) {
    std::vector<Move> moves;
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
            if (isCapture) m.captured = -1; // optional: wird später beim makeMove ermittelt
            moves.push_back(m);
        }
    };

    // Richtung abhängig von der Farbe
    int shift = (side == WHITE) ? 8 : -8;

    // Einfache Vorwärtszüge
    uint64_t singlePushes = (side == WHITE)
                                ? (pawns << 8) & ~board.allOccupied
                                : (pawns >> 8) & ~board.allOccupied;

    uint64_t temp = singlePushes;
    while (temp) {
        int to = bitScanForward(temp);
        temp &= temp - 1;
        int from = to - shift;

        const bool isPromotion = ((singlePushes & (1ULL << to)) & promoRank) != 0;

        if (isPromotion) {
            pushPromotionMoves(from, to, /*isCapture*/false);
        } else {
            Move m;
            m.from = from;
            m.to = to;
            m.moved = PAWN;
            m.flags = QUIET;
            moves.push_back(m);
        }
    }

    // Doppelzüge (nur von Grundreihe)
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
            moves.push_back(m);
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
            moves.push_back(m);
        }
    }

    // Schlagzüge (links/rechts diagonal)
    uint64_t enemyPieces = (side == WHITE) ? board.occupied[BLACK] : board.occupied[WHITE];

    uint64_t leftCaptures = (side == WHITE)
                                ? (pawns << 7) & enemyPieces & ~FILE_A // keine Wraps von a‑Linie
                                : (pawns >> 9) & enemyPieces & ~FILE_H; // keine Wraps von h‑Linie

    temp = leftCaptures;
    while (temp) {
        int to = bitScanForward(temp);
        temp &= temp - 1;
        int from = (side == WHITE) ? to - 7 : to + 9;

        const bool isPromotion = ((1ULL << to) & promoRank) != 0;
        if (isPromotion) {
            pushPromotionMoves(from, to, /*isCapture*/true);
        } else {
            Move m;
            m.from = from;
            m.to = to;
            m.moved = PAWN;
            m.flags = CAPTURE;
            moves.push_back(m);
        }
    }

    uint64_t rightCaptures = (side == WHITE)
                                 ? (pawns << 9) & enemyPieces & ~FILE_H // keine Wraps von h‑Linie
                                 : (pawns >> 7) & enemyPieces & ~FILE_A; // keine Wraps von a‑Linie

    temp = rightCaptures;
    while (temp) {
        int to = bitScanForward(temp);
        temp &= temp - 1;
        int from = (side == WHITE) ? to - 9 : to + 7;

        const bool isPromotion = ((1ULL << to) & promoRank) != 0;
        if (isPromotion) {
            pushPromotionMoves(from, to, /*isCapture*/true);
        } else {
            Move m;
            m.from = from;
            m.to = to;
            m.moved = PAWN;
            m.flags = CAPTURE;
            moves.push_back(m);
        }
    }

    // En Passant
    if (board.enPassantTarget != 0ULL) {
        uint64_t ep = board.enPassantTarget;

        if (side == WHITE) {
            uint64_t epLeft = (pawns << 7) & ep & ~FILE_A;
            uint64_t epRight = (pawns << 9) & ep & ~FILE_H;

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
                moves.push_back(m);
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
                moves.push_back(m);
            }
        } else {
            uint64_t epLeft = (pawns >> 9) & ep & ~FILE_H;
            uint64_t epRight = (pawns >> 7) & ep & ~FILE_A;

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
                moves.push_back(m);
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
                moves.push_back(m);
            }
        }
    }

    return moves;
}
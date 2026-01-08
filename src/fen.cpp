#include "../include/fen.h"
#include <cctype>
#include <sstream>

static void clearBoard(Board& b) {
    for (auto& colorArr : b.bitboards) {
        for (auto& bb : colorArr) bb = 0ULL;
    }
    b.occupied[WHITE] = 0ULL;
    b.occupied[BLACK] = 0ULL;
    b.allOccupied = 0ULL;

    // Neu
    b.kingSq[WHITE] = -1;
    b.kingSq[BLACK] = -1;

    b.sideToMove = WHITE;
    b.enPassantTarget = 0ULL;
    b.whiteKingsideCastle = false;
    b.whiteQueensideCastle = false;
    b.blackKingsideCastle = false;
    b.blackQueensideCastle = false;
}

static void recomputeOccupancy(Board& b) {
    b.occupied[WHITE] = 0ULL;
    b.occupied[BLACK] = 0ULL;
    for (int pt = PAWN; pt <= KING; ++pt) {
        b.occupied[WHITE] |= b.bitboards[WHITE][pt];
        b.occupied[BLACK] |= b.bitboards[BLACK][pt];
    }
    b.allOccupied = b.occupied[WHITE] | b.occupied[BLACK];
}

static bool setPiece(Board& b, char c, int square) {
    Color col = std::isupper(static_cast<unsigned char>(c)) ? WHITE : BLACK;
    char lc = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    PieceType pt;
    switch (lc) {
        case 'p': pt = PAWN; break;
        case 'n': pt = KNIGHT; break;
        case 'b': pt = BISHOP; break;
        case 'r': pt = ROOK; break;
        case 'q': pt = QUEEN; break;
        case 'k': pt = KING; break;
        default: return false;
    }

    b.bitboards[col][pt] |= (1ULL << square);

    // Neu: Königssquare direkt setzen
    if (pt == KING) {
        b.kingSq[col] = square;
    }

    return true;
}

static bool parseEnPassant(Board& b, const std::string& ep) {
    if (ep == "-") {
        b.enPassantTarget = 0ULL;
        return true;
    }
    if (ep.size() != 2) return false;

    char file = ep[0];
    char rank = ep[1];
    if (file < 'a' || file > 'h') return false;
    if (rank < '1' || rank > '8') return false;

    int f = file - 'a';
    int r = rank - '1';
    int square = r * 8 + f;
    b.enPassantTarget = (1ULL << square);
    return true;
}

bool loadFEN(Board& board, const std::string& fen) {
    clearBoard(board);

    std::istringstream iss(fen);
    std::string placement, stm, castling, ep;
    if (!(iss >> placement >> stm >> castling >> ep)) {
        return false;
    }

    // 1) Figurenbelegung
    int rank = 7;
    int file = 0;

    for (char c : placement) {
        if (c == '/') {
            if (file != 8) return false;
            rank--;
            file = 0;
            continue;
        }
        if (rank < 0) return false;

        if (std::isdigit(static_cast<unsigned char>(c))) {
            int empty = c - '0';
            if (empty < 1 || empty > 8) return false;
            file += empty;
            if (file > 8) return false;
        } else {
            if (file >= 8) return false;
            int square = rank * 8 + file;
            if (!setPiece(board, c, square)) return false;
            file++;
        }
    }
    if (rank != 0 || file != 8) {
        return false;
    }

    // 2) Side to move
    if (stm == "w") board.sideToMove = WHITE;
    else if (stm == "b") board.sideToMove = BLACK;
    else return false;

    // 3) Castling rights
    if (castling != "-") {
        for (char c : castling) {
            switch (c) {
                case 'K': board.whiteKingsideCastle = true; break;
                case 'Q': board.whiteQueensideCastle = true; break;
                case 'k': board.blackKingsideCastle = true; break;
                case 'q': board.blackQueensideCastle = true; break;
                default: return false;
            }
        }
    }

    // 4) En passant
    if (!parseEnPassant(board, ep)) return false;

    recomputeOccupancy(board);
    return true;
}
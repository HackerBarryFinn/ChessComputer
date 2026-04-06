#include "chess/rules/fen.h"
#include <cctype>
#include <sstream>

/**
 * Clears and resets all the fields of the given chess board to their default empty state.
 *
 * @param b A reference to the Board object that represents the chess game state.
 *          All board bitboards, occupancies, pieces, and metadata fields will
 *          be reset to their initial values.
 */
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

/**
 * Recomputes the occupancy bitboards for both colors and updates the combined board occupancy state.
 *
 * @param b A reference to the Board object whose occupancy bitboards are being
 *          recalculated. This includes updating the bitboards for individual
 *          color occupancies (WHITE and BLACK) as well as the overall occupancy
 *          of the board.
 */
static void recomputeOccupancy(Board& b) {
    b.occupied[WHITE] = 0ULL;
    b.occupied[BLACK] = 0ULL;
    for (int pt = PAWN; pt <= KING; ++pt) {
        b.occupied[WHITE] |= b.bitboards[WHITE][pt];
        b.occupied[BLACK] |= b.bitboards[BLACK][pt];
    }
    b.allOccupied = b.occupied[WHITE] | b.occupied[BLACK];
}

/**
 * Sets a chess piece on the specified square of the given board and updates
 * the board's state accordingly.
 *
 * @param b A reference to the Board object representing the chess game state.
 *          The specified piece type and its position will be reflected in the
 *          board's internal bitboards.
 * @param c A character representing the chess piece to place on the board.
 *          Uppercase characters ('P', 'N', 'B', 'R', 'Q', 'K') indicate white
 *          pieces, while lowercase characters ('p', 'n', 'b', 'r', 'q', 'k')
 *          indicate black pieces.
 * @param square An integer representing the target square (0-63) on the board
 *               where the piece should be placed.
 *
 * @return True if the piece was successfully placed; false if the character `c`
 *         does not correspond to a valid chess piece.
 */
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

/**
 * Parses the en passant field from a FEN string and updates the board's enPassantTarget field accordingly.
 *
 * @param b A reference to the Board object to update with the en passant square,
 *          if valid. The enPassantTarget will be set to the appropriate bitboard
 *          representation of the square, or cleared if no en passant is available.
 * @param ep A string representing the en passant target square in algebraic
 *           notation (e.g., "e6"), or "-" if no en passant square is available.
 * @return True if the en passant field is valid and has been successfully parsed,
 *         false otherwise.
 */
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

/**
 * Loads a chess position from a given FEN string and updates the specified board representation accordingly.
 *
 * @param board A reference to the Board object that represents the chess game
 *              state. This board will be reset and updated based on the FEN provided.
 * @param fen A string containing the FEN representation of a chess position, including
 *            piece placement, active player, castling rights, en passant targets,
 *            and other metadata.
 * @return A boolean value indicating success or failure. Returns true if the FEN
 *         was parsed and loaded successfully. Returns false if the FEN is invalid
 *         or cannot be loaded.
 */
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
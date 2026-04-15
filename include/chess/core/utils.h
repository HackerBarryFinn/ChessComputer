#pragma once

#include <string>

#ifdef _MSC_VER
#include <intrin.h>
#pragma intrinsic(_BitScanForward64)

inline int bitScanForward(uint64_t bb) {
    unsigned long index;
    _BitScanForward64(&index, bb);
    return static_cast<int>(index);
}

#else
inline int bitScanForward(uint64_t bb) {
    return __builtin_ctzll(bb); // GCC/Clang Builtin
}
#endif

// --- Helpers ---
inline std::string squareToString(int sq) {
    int file = sq % 8;
    int rank = sq / 8;
    std::string s;
    s += static_cast<char>('a' + file);
    s += static_cast<char>('1' + rank);
    return s;
}

inline char pieceToChar(int pt) {
    switch (pt) {
        case 0: return 'P';
        case 1: return 'N';
        case 2: return 'B';
        case 3: return 'R';
        case 4: return 'Q';
        case 5: return 'K';
        default: return '-';
    }
}

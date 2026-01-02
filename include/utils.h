#pragma once
#include <cstdint>

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

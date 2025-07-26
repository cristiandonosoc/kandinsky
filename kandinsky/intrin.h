#pragma once

#include <kandinsky/defines.h>

#ifdef COMPILER_MSVC

#include <intrin.h>

// Assumes the value is non-zero.
inline i32 BitScanForward(i32 value) {
    unsigned long index;
    _BitScanForward(&index, (unsigned long)value);
    return (i32)index;
}

#endif  // COMPILER_MSVC

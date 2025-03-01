#pragma once

#include <kandinsky/defines.h>

namespace kdk {

struct Arena;

// Uses djb2 for now.
// http://www.cse.yorku.ca/~oz/hash.html
u32 HashString(const char* string);

// Returns hash + 1 so we can use 0 as none;
inline u32 IDFromString(const char* string) { return HashString(string) + 1; }

const char* Printf(Arena* arena, const char* fmt, ...);

void PrintBacktrace(Arena* arena, u32 frames_to_skip = 0);

// Useful for printing line numbers without the bazel nonesense.
const char* CleanPathFromBazel(const char* path);

}  // namespace kdk

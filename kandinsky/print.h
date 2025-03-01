#pragma once

#include <kandinsky/defines.h>

namespace kdk {

struct Arena;

const char* Printf(Arena* arena, const char* fmt, ...);

void PrintBacktrace(Arena* arena, u32 frames_to_skip = 0);

// Useful for printing line numbers without the bazel nonesense.
const char* CleanPathFromBazel(const char* path);

}  // namespace kdk

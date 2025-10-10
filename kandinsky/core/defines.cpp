#include <kandinsky/core/defines.h>

#include <kandinsky/core/memory.h>
#include <kandinsky/core/string.h>

#include <stb/stb_sprintf.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace kdk {
namespace internal {

namespace defines_private {

void DoAssert(Arena* arena,
              const char* expr,
              const char* filename,
              u32 lineno,
              const char* function,
              const char* message) {
    filename = paths::CleanPathFromBazel(filename);
    fprintf(stderr, "***ASSERT FAILED *********************************************************\n");
    fprintf(stderr, "Expr: %s\n", expr);
    fprintf(stderr, "At: %s (%s:%d)\n", function, filename, lineno);
    if (message) {
        fprintf(stderr, "Message: %s\n", message);
    }

    PrintBacktrace(arena, 2);
    std::abort();
}

}  // namespace defines_private

void HandleAssert(const char* expr, const char* filename, u32 lineno, const char* function) {
    // Create an arena on the fly (the program is dying anyway).
    Arena arena = AllocateArena("AssertArena"sv, 64 * KILOBYTE);
    defines_private::DoAssert(&arena, expr, filename, lineno, function, nullptr);
}

void HandleAssertf(const char* expr,
                   const char* filename,
                   u32 lineno,
                   const char* function,
                   const char* fmt,
                   ...) {
    // Create an arena on the fly (the program is dying anyway).
    Arena arena = AllocateArena("AssertfArena"sv, 64 * KILOBYTE);

    char* message = (char*)ArenaPush(&arena, 2 * STB_SPRINTF_MIN);
    va_list va;
    va_start(va, fmt);
    stbsp_vsprintf(message, fmt, va);
    va_end(va);

    defines_private::DoAssert(&arena, expr, filename, lineno, function, message);
}

}  // namespace internal
}  // namespace kdk

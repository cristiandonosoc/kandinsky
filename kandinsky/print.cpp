#include <kandinsky/print.h>

#include <kandinsky/memory.h>

#include <stb/stb_sprintf.h>

namespace kdk {

const char* Printf(Arena* arena, const char* fmt, ...) {
    u8* buf = ArenaPush(arena, 2 * STB_SPRINTF_MIN);
    va_list va;
    va_start(va, fmt);
    stbsp_vsprintf((char*)buf, fmt, va);
    va_end(va);

    return (const char*)buf;
}

}  // namespace kdk

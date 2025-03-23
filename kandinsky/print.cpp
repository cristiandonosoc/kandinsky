#include <kandinsky/print.h>

#include <kandinsky/memory.h>

#include <stb/stb_sprintf.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <dbghelp.h>

#include <array>
#include <cstdio>

namespace kdk {

const char* Printf(Arena* arena, const char* fmt, ...) {
    int size = 2 * STB_SPRINTF_MIN;
    char* buf = (char*)ArenaPush(arena, size);
    va_list va;
    va_start(va, fmt);
    stbsp_vsnprintf(buf, size, fmt, va);
    va_end(va);

    return buf;
}

void PrintBacktrace(Arena* arena, u32 frames_to_skip) {
    // We don't want to get this function in the way.
    frames_to_skip++;

    constexpr u32 kFramesToCapture = 16;
    std::array<void*, kFramesToCapture> frames;
    u32 frame_count = CaptureStackBackTrace(frames_to_skip, 16, frames.data(), NULL);

    HANDLE handle = GetCurrentProcess();
    if (!SymInitialize(handle, nullptr, true)) {
        char buffer[256];

        // Format the error message
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       GetLastError(),
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       buffer,
                       256,
                       NULL);
        std::printf("ERROR: PrintBacktrace: SymInitialize: %s\n", buffer);
        return;
    }

    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);

    // Symbol info buffer.
    SYMBOL_INFO* symbol = (SYMBOL_INFO*)ArenaPushZero(arena, sizeof(SYMBOL_INFO) + 256);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    IMAGEHLP_LINE64 line = {};
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    std::printf("--- BACKTRACE ----------------------------------------------------------------\n");

    bool has_seen_valid_frame = false;
    u32 frames_skipped = 0;
    for (u32 i = 0; i < frame_count; i++) {
        u64 addr = (u64)frames[i];

        const char* function_name = nullptr;
        if (SymFromAddr(handle, addr, nullptr, symbol)) {
            function_name = symbol->Name;
        }

        if (!function_name) {
            // We don't want to show a bunch of "<unknown>" at the top if we can avoid it.
            if (has_seen_valid_frame) {
                std::printf("Frame %02d: <unknown>\n", i);
            } else {
                frames_skipped++;
            }
            continue;
        }

        if (!has_seen_valid_frame) {
            if (frames_skipped > 0) {
                std::printf("Skipped %d frames (were \"<unknown>\")\n", frames_skipped);
            }
            has_seen_valid_frame = true;
        }

        // Get the file and line info.
        const char* file = "<unknown>";
        u32 line_number = 0;
        DWORD displacement = 0;
        if (SymGetLineFromAddr64(handle, addr, &displacement, &line)) {
            file = line.FileName;
            line_number = (u32)line.LineNumber;

            file = CleanPathFromBazel(file);
        }

        std::printf("Frame %02d: %s (%s:%d)\n",
                    i - frames_skipped,
                    function_name,
                    file,
                    line_number);
    }
}

const char* CleanPathFromBazel(const char* path) {
    // Remove bazel nonesense.
    const char* bazel_marker = "_main\\";
    if (const char* marker_pos = strstr(path, bazel_marker)) {
        path = marker_pos + strlen(bazel_marker);
    }
    return path;
}

}  // namespace kdk

#include <kandinsky/string.h>

#include <kandinsky/memory.h>

#include <cwalk.h>

#include <cstring>

namespace kdk {

u32 HashString(const char* string) {
    u32 hash = 5381;
    int c;

    while ((c = *string++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    return hash;
}

const char* InternStringToArena(Arena* arena, const char* string, u64 length) {
    if (length == 0) {
        length = std::strlen(string);
    }

    char* dst = (char*)ArenaPush(arena, length + 1);
    std::memcpy(dst, string, length + 1);
    return dst;
}

namespace paths {

String GetDirname(Arena* arena, String path) {
    u64 size = 0;
    if (cwk_path_get_dirname(path.Str , &size); size == 0) {
        return {};
    }

    const char* str = InternStringToArena(arena, path.Str , size);
    return String(str, size);
}

String GetBasename(Arena* arena, String path) {
    const char* out = nullptr;
    u64 size = 0;
    if (cwk_path_get_basename(path.Str , &out, &size); size == 0) {
        return {};
    }

    const char* str = InternStringToArena(arena, out, size);
    return String(str, size);
}

String GetExtension(Arena* arena, String path) {
    const char* extension = nullptr;
    u64 size = 0;
    if (cwk_path_get_extension(path.Str , &extension, &size); size == 0) {
        return {};
    }

    const char* str = InternStringToArena(arena, extension, size);
    return String(str, size);
}

String RemoveExtension(Arena* arena, String path) {
    String extension = GetExtension(arena, path);
    if (extension.IsEmpty()) {
        return path;
    }

    return String(path.Str , path.Size - extension.Size);
}

}  // namespace paths

}  // namespace kdk

#include <kandinsky/string.h>

#include <kandinsky/memory.h>

#include <cwalk.h>

#include <cstring>

namespace kdk {

const char* String::kEmptyStrPtr = "";

namespace string_private {

inline bool StrCmpWithLength(const char* s1, const char* s2, u64 size) {
    for (u64 i = 0; i < size; i++) {
        if (*s1++ != *s2++) {
            return false;
        }
    }

    return true;
}

}  // namespace string_private

bool String::Equals(const char* str) const {
    if (!IsValid()) {
        return false;
    }

    if (!str) {
        return false;
    }

    u64 size = std::strlen(str);
    if (Size != size) {
        return false;
    }

    if (_Str == str) {
        return true;
    }

    return string_private::StrCmpWithLength(_Str, str, Size);
}

bool String::Equals(const String& other) const {
    if (Size != other.Size) {
        return false;
    }

    // Since they are the same size, pointer comparison would mean they're equal.
    if (_Str == other._Str) {
        return true;
    }

    return string_private::StrCmpWithLength(_Str, other._Str, Size);
}

u32 HashString(const char* string) {
    u32 hash = 5381;

    while (true) {
        int c = *string++;
        if (c == 0) {
            break;
        }
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    return hash;
}

const char* InternStringToArena(Arena* arena, const char* string, u64 length) {
    if (length == 0) {
        length = std::strlen(string);
    }

    char* dst = (char*)ArenaPush(arena, length + 1);
    std::memcpy(dst, string, length);
    dst[length] = 0;  // The null terminator.
    return dst;
}

namespace paths {

String GetDirname(Arena* arena, String path) {
    if (path.IsEmpty()) {
        return {};
    }

    u64 size = 0;
    if (cwk_path_get_dirname(path.Str(), &size); size == 0) {
        return {};
    }

    const char* str = InternStringToArena(arena, path.Str(), size);
    return String(str, size);
}

String GetBasename(Arena* arena, String path) {
    if (path.IsEmpty()) {
        return {};
    }

    const char* out = nullptr;
    u64 size = 0;
    if (cwk_path_get_basename(path.Str(), &out, &size); size == 0) {
        return {};
    }

    const char* str = InternStringToArena(arena, out, size);
    return String(str, size);
}

String GetExtension(Arena* arena, String path) {
    if (path.IsEmpty()) {
        return {};
    }

    const char* extension = nullptr;
    u64 size = 0;
    if (cwk_path_get_extension(path.Str(), &extension, &size); size == 0) {
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

    // Special case where extension is the same as the input (hidden files).
    if (extension.Size == path.Size) {
        return path;
    }

    return String(path.Str(), path.Size - extension.Size);
}

String PathJoin(Arena* arena, String a, String b) {
    if (a.IsEmpty()) {
        return b;
    }

    if (b.IsEmpty()) {
        return a;
    }

    // Worst case we string them together.
    u64 buffer_size = a.Size + b.Size + 2;
    char* buffer = (char*)ArenaPush(arena, buffer_size);
    u64 size = 0;
    if (size = cwk_path_join(a.Str(), b.Str(), buffer, buffer_size); size == 0) {
        return {};
    }

    return String(buffer, size);
}

}  // namespace paths

}  // namespace kdk

#pragma once

#include <kandinsky/defines.h>

#include <cstring>

namespace kdk {

struct Arena;

struct String {
    const char* Str = nullptr;
    u64 Size = 0;

    String() {}
    explicit String(const char* str) : Str (str), Size(std::strlen(str)) {}
	explicit String(const char* str, u64 size) : Str (str), Size(size) {}

    bool IsEmpty() const { return Size == 0; }
};

// Uses djb2 for now.
// http://www.cse.yorku.ca/~oz/hash.html
u32 HashString(const char* string);

// Returns hash + 1 so we can use 0 as none;
inline u32 IDFromString(const char* string) { return HashString(string) + 1; }

// |length| MUST NOT include the zero terminator.
const char* InternStringToArena(Arena* arena, const char* string, u64 length = 0);

// Paths -------------------------------------------------------------------------------------------

namespace paths {

String GetDirname(Arena* arena, String path);
String GetBasename(Arena* arena, String path);
String GetExtension(Arena* arena, String path);
String RemoveExtension(Arena* arena, String path);

}  // namespace paths

}  // namespace kdk

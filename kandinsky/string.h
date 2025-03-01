#pragma once

#include <kandinsky/defines.h>

#include <cstring>

namespace kdk {

struct Arena;

struct String {
    static const char* kEmptyStrPtr;

    // You should not get this pointer directly if you want to use it for printing, since it might
    // be null. Use |Str()| instead.
    const char* _Str = nullptr;
    u64 Size = 0;

    // By default we create the empty value rather than null.
    // Easier for comparisons.
    String() : _Str(kEmptyStrPtr), Size(0) {}
    explicit String(const char* str) : _Str(str), Size(std::strlen(str)) {}
    explicit String(const char* str, u64 size) : _Str(str), Size(size) {}

    const char* Str() const { return _Str ? _Str : kEmptyStrPtr; }

    bool IsEmpty() const { return Size == 0; }
    bool IsValid() const { return _Str != nullptr; }

    bool Equals(const char* str) const;
    bool Equals(const String& other) const;
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
String PathJoin(Arena* arena, String a, String b);

}  // namespace paths

}  // namespace kdk

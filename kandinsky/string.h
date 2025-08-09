#pragma once

#include <kandinsky/defines.h>

#include <SDL3/SDL_filesystem.h>

#include <cstring>
#include <span>

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

    bool operator==(const String& other) const { return Equals(other); }
};

template <u64 CAPACITY>
struct FixedString {
    std::array<char, CAPACITY> Data = {};
    u32 Size = 0;

    FixedString() { Set(String()); }  // Default to empty string.
    FixedString(const char* str) { Set(str); }
    FixedString(String string) { Set(string); }

    void Set(const char* str) { Set(String(str)); }
    void Set(String string) {
        Size = (u32)string.Size;
        if (Size >= CAPACITY) {
            Size = CAPACITY - 1;  // Leave space for null terminator.
        }
        std::memcpy(Data.data(), string.Str(), Size);
        Data[Size] = '\0';
    }

    String ToString() const { return String(Data.data(), Size); }
    const char* Str() const { return ToString().Str(); }
};

// Uses djb2 for now.
// http://www.cse.yorku.ca/~oz/hash.html
constexpr u32 CompileHash(const char* string) {
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

inline u32 HashString(const char* string) { return CompileHash(string); }

// Literal operator for convenient usage with string literals
constexpr uint32_t operator"" _hash(const char* str, size_t) { return CompileHash(str); }

// Returns hash + 1 so we can use 0 as none;
inline u32 IDFromString(const char* string) { return HashString(string) + 1; }
inline u32 IDFromString(const String& string) { return IDFromString(string.Str()); }

// |length| MUST NOT include the zero terminator.
const char* InternStringToArena(Arena* arena, const char* string, u64 length = 0);

String Concat(Arena* arena, String a, String b);

// Printf ------------------------------------------------------------------------------------------

String Printf(Arena* arena, const char* fmt, ...);

void PrintBacktrace(Arena* arena, u32 frames_to_skip = 0);

// Paths -------------------------------------------------------------------------------------------

namespace paths {

bool IsAbsolute(const char* path);
inline bool IsAbsolute(const String& path) { return IsAbsolute(path.Str()); }

// The directory this program was run from.
String GetBaseDir(Arena* arena);

String GetDirname(Arena* arena, String path);
String GetBasename(Arena* arena, String path);
String GetExtension(Arena* arena, String path);
String RemoveExtension(Arena* arena, String path);

inline String PathJoin(Arena*, String a) { return a; }  // Base case for recursion.
String PathJoin(Arena* arena, String a, String b);

// Recursive variadic template to handle arbitrary number of paths
template <typename... Paths>
String PathJoin(Arena* arena, String first, String second, Paths... rest) {
    // Join the first two paths, then recursively join with the rest
    // TODO(cdc): This is very dumb, as it will allocate every sub-path, but this ok for now.
    return PathJoin(arena, PathJoin(arena, first, second), rest...);
}

struct DirEntry {
    String Path = {};
    SDL_PathInfo Info = {};

    bool IsFile() const { return Info.type == SDL_PATHTYPE_FILE; }
    bool IsDir() const { return Info.type == SDL_PATHTYPE_DIRECTORY; }
};

std::span<DirEntry> ListDir(Arena* arena, String path);

// Useful for printing line numbers without the bazel nonesense.
// TODO(cdc): Change it to use String.
const char* CleanPathFromBazel(const char* path);

}  // namespace paths

// System ------------------------------------------------------------------------------------------
// TODO(cdc): Move to a more "system" like place.

String GetEnv(Arena* arena, const char* env);

}  // namespace kdk

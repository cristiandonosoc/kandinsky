#pragma once

#include <kandinsky/defines.h>

#include <SDL3/SDL_filesystem.h>

#include <cstring>

namespace kdk {

struct Arena;

template <typename T>
struct Array {
    T* Entries = nullptr;
    u32 Count = 0;
};

template <typename T>
inline bool IsValid(const Array<T>& a) {
    return a.Entries != nullptr && a.Count > 0;
}

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

Array<DirEntry> ListDir(Arena* arena, String path);

}  // namespace paths

const char* GetEnv(Arena* arena, const char* env);

}  // namespace kdk

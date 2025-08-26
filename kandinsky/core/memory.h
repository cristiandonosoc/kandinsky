#pragma once

#include <kandinsky/core/defines.h>
#include <kandinsky/core/string.h>

#include <span>

namespace kdk {

struct PlatformState;

// System ------------------------------------------------------------------------------------------

constexpr u64 BYTE = 1;
constexpr u64 KILOBYTE = 1024 * BYTE;
constexpr u64 MEGABYTE = 1024 * KILOBYTE;
constexpr u64 GIGABYTE = 1024 * MEGABYTE;
constexpr u64 TERABYTE = 1024 * GIGABYTE;

// Arenas ------------------------------------------------------------------------------------------

enum class EArenaType : u8 {
    // Simple one buffer arena. Traps if the size is exceeded.
    FixedSize,
    // Starts as fixed size, but when the next allocation would overflow, it will allocate a new
    // arena of the same size and "chain" to it.
    // Note that uses some memory at the end of the buffer for the link data structure, so not all
    // reported memory is available.
    Extendable,
};

// Non-copyable, Non-movable RAII style temporary arena.
// This is meant to be used in the scope of a stack frame only.
struct ScratchArena {
    Arena* Arena = nullptr;
    u64 OriginalOffset = 0;

    struct Arena* GetPtr() { return Arena; }
    operator struct Arena *() { return Arena; }

    explicit ScratchArena(struct Arena* arena, u64 original_offset);
    ~ScratchArena();

    ScratchArena(const ScratchArena&) = delete;
    ScratchArena& operator=(const ScratchArena&) = delete;

    ScratchArena(ScratchArena&&) = delete;
    ScratchArena& operator=(ScratchArena&&) = delete;
};

struct Arena {
    u8* Start = nullptr;
    // NOTE: Shows the size of this particular arena.
    //       In the case of Extendable arenas, this represents only that "link".
    //       To know the total size of the arena, refer to |ExtendableData.TotalSize|.
    u64 Size = 0;
    u64 Offset = 0;

    EArenaType Type = EArenaType::FixedSize;

    struct Stats {
        u32 AllocCalls = 0;
        u32 FreeCalls = 0;
    } Stats = {};

    union {
        struct {
        } FixedData;
        struct {
            Arena* NextArena = nullptr;
            // The size of _this_ link.
            u64 MaxLinkOffset = 0;
            u64 TotalSize = 0;
        } ExtendableData;
    };

    Arena* GetPtr() { return this; }
    ScratchArena GetScopedArena() { return ScratchArena(this, Offset); }
};
bool IsValid(const Arena& arena);

Arena AllocateArena(u64 size, EArenaType type = EArenaType::FixedSize);
void FreeArena(Arena* arena);

void ArenaReset(Arena* arena);
u8* ArenaPush(Arena* arena, u64 size, u64 alignment = 8);
u8* ArenaPushZero(Arena* arena, u64 size, u64 alignment = 8);

template <typename T>
T* ArenaPush(Arena* arena) {
    return (T*)ArenaPush(arena, sizeof(T), alignof(T));
}

template <typename T>
T* ArenaPushZero(Arena* arena) {
    return (T*)ArenaPushZero(arena, sizeof(T), alignof(T));
}

// Allocates memory and calls the struct initializer.
template <typename T>
T* ArenaPushInit(Arena* arena) {
    T* t = (T*)ArenaPushZero(arena, sizeof(T), alignof(T));
    new (t) T;  // Placement new.
    return t;
}

template <typename T>
std::span<T> ArenaPushArray(Arena* arena, u64 count) {
    T* t = (T*)ArenaPush(arena, count * sizeof(T), alignof(T));
    return {t, count};
}

// Copy into the arena some data.
inline std::span<u8> ArenaCopy(Arena* arena, std::span<u8> data, u64 alignment = 8) {
    u8* ptr = ArenaPush(arena, data.size_bytes(), alignment);
    if (ptr) {
        std::memcpy(ptr, data.data(), data.size_bytes());
    }
    return {ptr, data.size_bytes()};
}

ScratchArena GetScratchArena(Arena* conflict1 = nullptr, Arena* conflict2 = nullptr);

// Use for testing.
std::span<Arena> ReferenceScratchArenas();

// Memory Alignment --------------------------------------------------------------------------------

inline bool IsPowerOf2(u64 a) { return ((a & (a - 1)) == 0); }
void* Align(void* ptr, u64 alignment);
void* AlignForward(void* ptr, u64 alignment);

// UTILITIES ---------------------------------------------------------------------------------------

String ToMemoryString(Arena* arena, u32 bytes);

}  // namespace kdk

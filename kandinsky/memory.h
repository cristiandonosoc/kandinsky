#pragma once

#include <kandinsky/defines.h>
#include <kandinsky/string.h>

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
T* ArenaPushArray(Arena* arena, u64 count) {
    return (T*)ArenaPush(arena, count * sizeof(T), alignof(T));
}

// Non-copyable, Non-movable RAII style temporary arena.
// This is meant to be used in the scope of a stack frame only.
struct ScratchArena {
    Arena* Arena = nullptr;
    u64 OriginalOffset = 0;

    ScratchArena(struct Arena* arena, u64 original_offset);
    ~ScratchArena();

    ScratchArena(const ScratchArena&) = delete;
    ScratchArena& operator=(const ScratchArena&) = delete;

    ScratchArena(ScratchArena&&) = delete;
    ScratchArena& operator=(ScratchArena&&) = delete;
};

ScratchArena GetScratchArena(Arena* conflict1 = nullptr, Arena* conflict2 = nullptr);

// Use for testing.
std::span<Arena> ReferenceScratchArenas();

// Memory Alignment --------------------------------------------------------------------------------

inline bool IsPowerOf2(u64 a) { return ((a & (a - 1)) == 0); }
void* Align(void* ptr, u64 alignment);
void* AlignForward(void* ptr, u64 alignment);

// UTILITIES ---------------------------------------------------------------------------------------

String ToMemoryString(u32 bytes);

}  // namespace kdk

#pragma once

#include <kandinsky/defines.h>

#include <functional>

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
    } Stats;

    union {
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

// String specific.
const char* InternString(Arena* arena, const char* string);

// Memory Alignment --------------------------------------------------------------------------------

inline bool IsPowerOf2(u64 a) { return ((a & (a - 1)) == 0); }
void* Align(void* ptr, u64 alignment);
void* AlignForward(void* ptr, u64 alignment);

}  // namespace kdk

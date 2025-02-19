#include <kandinsky/memory.h>

#include <kandinsky/platform.h>

#include <string.h>
#include <cassert>
#include <cstdlib>
#include <cstring>

namespace kdk {

namespace memory_private {

void FreeExtendableArena(Arena* arena) {
    assert(IsValid(*arena));

    if (arena->ExtendableData.NextArena) {
        FreeExtendableArena(arena->ExtendableData.NextArena);
    }

    free(arena->Start);
    *arena = {};
}

}  // namespace memory_private

bool IsValid(const Arena& arena) {
    if (!arena.Start) {
        return false;
    }

    if (arena.Size == 0) {
        return false;
    }

    if (arena.Offset >= arena.Size) {
        return false;
    }

    return true;
}

Arena AllocateArena(u64 size, EArenaType type) {
    u8* start = (u8*)malloc(size);

    return Arena{
        .Start = start,
        .Size = size,
        .Offset = 0,
        .Type = type,
    };
}

void FreeArena(Arena* arena) {
    using namespace memory_private;

    assert(IsValid(*arena));

    switch (arena->Type) {
        case EArenaType::FixedSize: {
            free(arena->Start);
            *arena = {};
            return;
        }
        case EArenaType::Extendable: {
            FreeExtendableArena(arena);
            return;
        }
    }

    assert(false);
}

u8* ArenaPush(Arena* arena, u64 size, u64 aligment) {
    // Determine the new offset
    u8* ptr = arena->Start + arena->Offset;
    ptr = (u8*)AlignForward(ptr, aligment);

    u64 offset = ptr - arena->Start;

    if (offset + size >= arena->Size) {
        assert(false);
    }

    arena->Offset = offset + size;
    return ptr;
}

const char* InternString(Arena* arena, const char* string) {
    u64 len = std::strlen(string) + 1;  // Extra byte for the null terminator.
    u8* ptr = ArenaPush(arena, len);
    std::memcpy(ptr, string, len);
    return (const char*)ptr;
}

u8* ArenaPushZero(Arena* arena, u64 size, u64 aligment) {
    u8* ptr = ArenaPush(arena, size, aligment);
    std::memset(ptr, 0, size);
    return ptr;
}

bool InitMemory(PlatformState* ps) {
    ps->Memory.FrameArena = AllocateArena(100 * MEGABYTE);
    ps->Memory.StringArena = AllocateArena(100 * MEGABYTE);

    return true;
}

void ShutdownMemory(PlatformState* ps) {
    FreeArena(&ps->Memory.StringArena);
    FreeArena(&ps->Memory.FrameArena);
}

void* Align(void* ptr, u64 alignment) {
    assert(IsPowerOf2(alignment));

    u64 v = (u64)ptr;
    u64 mask = alignment - 1;

    // Clear the least significant bits up to alignment using bitwise AND with inverted mask.
    v &= ~mask;

    return (void*)v;
}

void* AlignForward(void* ptr, u64 alignment) {
    assert(IsPowerOf2(alignment));

    // Same as (p % a), but faster since alignment is power of 2.
    u64 v = (u64)ptr;
    u64 mask = alignment - 1;
    u64 modulo = v & mask;

    if (modulo != 0) {
        v += alignment - modulo;
    }

    return (void*)v;
}

}  // namespace kdk

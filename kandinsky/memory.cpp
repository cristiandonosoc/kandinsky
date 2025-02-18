#include <kandinsky/memory.h>

#include <kandinsky/platform.h>

#include <string.h>
#include <cassert>
#include <cstdlib>
#include <cstring>

namespace kdk {

namespace memory_private {

bool IsPowerOf2(u64 a) { return ((a & (a - 1)) == 0); }

u8* AlignForward(void* ptr, u64 aligment) {
    assert(IsPowerOf2(aligment));

    // Same as (p % a), but faster since aligment is power of 2.
    u64 v = (u64)ptr;
    u64 modulo = v & (aligment - 1);

    if (modulo != 0) {
        v += aligment - modulo;
    }

    return (u8*)v;
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

Arena AllocateArena(u64 size) {
    u8* start = (u8*)malloc(size);

    return Arena{
        .Start = start,
        .Size = size,
        .Offset = 0,
    };
}

void FreeArena(Arena* arena) {
    assert(IsValid(*arena));

    free(arena->Start);
    *arena = {};
}

u8* ArenaPush(Arena* arena, u64 size, u64 aligment) {
    u8* ptr = arena->Start + arena->Offset;
    ptr = memory_private::AlignForward(ptr, aligment);

    u64 offset = ptr - arena->Start;

    if (offset + size >= arena->Size) {
        // TODO(cdc): Handle this more gracefully.
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

}  // namespace kdk

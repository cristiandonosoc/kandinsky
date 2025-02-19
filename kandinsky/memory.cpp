#include <kandinsky/memory.h>

#include <kandinsky/platform.h>

#include <string.h>
#include <cassert>
#include <cstdlib>
#include <cstring>

namespace kdk {

namespace memory_private {

Arena AllocExtendableArena(u64 size) {
    u8* start = (u8*)malloc(size);
    Arena arena{
        .Start = start,
        .Size = size,
        .Offset = 0,
        .Type = EArenaType::Extendable,
    };

    // We "embed" the next link at the end of the buffer.
    u64 max_offset = (u64)Align((void*)(size - sizeof(Arena)), 8);
    arena.ExtendableData.NextArena = nullptr;
    arena.ExtendableData.MaxLinkOffset = max_offset;

    return arena;
}

void FreeExtendableArena(Arena* arena) {
    assert(IsValid(*arena));

    if (arena->ExtendableData.NextArena) {
        FreeExtendableArena(arena->ExtendableData.NextArena);
    }

    free(arena->Start);
    *arena = {};
}

u8* ExtendableArenaPush(Arena* arena, u64 size, u64 alignment) {
    // Determine the new offset
    u8* ptr = arena->Start + arena->Offset;
    ptr = (u8*)AlignForward(ptr, alignment);
    u64 offset = ptr - arena->Start;

#if ENABLE_FOR_DEBUGGING
    printf("Offset: %llu, Size: %llu, Offset + Size: %llu, MaxLinkOffset: %llu\n",
           offset,
           size,
           offset + size,
           arena->ExtendableData.MaxLinkOffset);
#endif

    // Check if this link is full.
    if (offset + size > arena->ExtendableData.MaxLinkOffset) {
        // If we're full, check if we have a link already. If not, we create one.
        if (!arena->ExtendableData.NextArena) {
            // We allocate it *exactly* where |MaxLinkOffset| is, since we created an aligned
            // space for it.
            Arena* new_arena = (Arena*)(arena->Start + arena->ExtendableData.MaxLinkOffset);
            *new_arena = AllocExtendableArena(arena->Size);
            arena->ExtendableData.NextArena = new_arena;
        }

        return ExtendableArenaPush(arena->ExtendableData.NextArena, size, alignment);
    }

    // Otherwise we alloc as normal.
    arena->Offset = offset + size;
    return ptr;
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

u64 GetArenaSize(Arena* arena) {
    assert(IsValid(*arena));

    switch (arena->Type) {
        case EArenaType::FixedSize: {
            return arena->Size;
        }
        case EArenaType::Extendable: {
            u64 total = arena->Size;
            Arena* next_arena = arena->ExtendableData.NextArena;
            while (next_arena) {
                total += next_arena->Size;
                next_arena = next_arena->ExtendableData.NextArena;
            }

            return total;
        }
    }

    assert(false);
    return 0;
}

Arena AllocateArena(u64 size, EArenaType type) {
    switch (type) {
        case EArenaType::FixedSize: {
            u8* start = (u8*)malloc(size);
            return Arena{
                .Start = start,
                .Size = size,
                .Offset = 0,
                .Type = type,
            };
        }
        case EArenaType::Extendable: {
            return memory_private::AllocExtendableArena(size);
        }
    }

    assert(false);
    return {};
}

void FreeArena(Arena* arena) {
    assert(IsValid(*arena));

    switch (arena->Type) {
        case EArenaType::FixedSize: {
            free(arena->Start);
            *arena = {};
            return;
        }
        case EArenaType::Extendable: {
            memory_private::FreeExtendableArena(arena);
            return;
        }
    }

    assert(false);
}

u8* ArenaPush(Arena* arena, u64 size, u64 alignment) {
    assert(IsValid(*arena));

    if (size > GetArenaSize(arena)) {
        DEBUG_BREAK();
        return nullptr;
    }

    switch (arena->Type) {
        case EArenaType::FixedSize: {
            // Determine the new offset
            u8* ptr = arena->Start + arena->Offset;
            ptr = (u8*)AlignForward(ptr, alignment);
            u64 offset = ptr - arena->Start;

            assert(offset + size < arena->Size);

            arena->Offset = offset + size;
            return ptr;
        }
        case EArenaType::Extendable: {
            return memory_private::ExtendableArenaPush(arena, size, alignment);
        }
    }

    assert(false);
    return nullptr;
}

const char* InternString(Arena* arena, const char* string) {
    u64 len = std::strlen(string) + 1;  // Extra byte for the null terminator.
    u8* ptr = ArenaPush(arena, len);
    std::memcpy(ptr, string, len);
    return (const char*)ptr;
}

u8* ArenaPushZero(Arena* arena, u64 size, u64 alignment) {
    u8* ptr = ArenaPush(arena, size, alignment);
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

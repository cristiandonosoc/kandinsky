#include <kandinsky/memory.h>

#include <string.h>
#include <cstdlib>
#include <cstring>

namespace kdk {

namespace memory_private {

void* AllocMemory(Arena* arena, u64 size) {
    arena->Stats.AllocCalls++;
    return malloc(size);
}

void FreeMemory(Arena* arena, void* ptr) {
    arena->Stats.FreeCalls++;
    free(ptr);
}

Arena AllocExtendableArena(u64 size) {
    Arena arena{
        .Size = size,
        .Offset = 0,
        .Type = EArenaType::Extendable,
    };
    arena.Start = (u8*)AllocMemory(&arena, size);

    // We "embed" the next link at the end of the buffer.
    u64 max_offset = (u64)Align((void*)(size - sizeof(Arena)), 8);
    arena.ExtendableData.NextArena = nullptr;
    arena.ExtendableData.MaxLinkOffset = max_offset;

    return arena;
}

// Return free calls.
u32 FreeExtendableArena(Arena* arena) {
    ASSERT(IsValid(*arena));

    u32 free_calls = 0;
    if (arena->ExtendableData.NextArena) {
        free_calls += FreeExtendableArena(arena->ExtendableData.NextArena);
    }

    FreeMemory(arena, arena->Start);
    free_calls++;
    *arena = {};

    return free_calls;
}

u8* ExtendableArenaPush(Arena* arena, u64 size, u64 alignment) {
    ASSERT(size <= arena->ExtendableData.MaxLinkOffset);

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

void ConsolidateNumbers(Arena* arena) {
    switch (arena->Type) {
        case EArenaType::FixedSize:
            return;
        case EArenaType::Extendable: {
            Arena* next_arena = arena->ExtendableData.NextArena;

            arena->ExtendableData.TotalSize = arena->Size;
            while (next_arena) {
                arena->ExtendableData.TotalSize += next_arena->Size;
                next_arena->ExtendableData.TotalSize = 0;
                arena->Stats.AllocCalls += next_arena->Stats.AllocCalls;
                next_arena->Stats.AllocCalls = 0;
                arena->Stats.FreeCalls += next_arena->Stats.AllocCalls;
                next_arena->Stats.FreeCalls = 0;

                next_arena = next_arena->ExtendableData.NextArena;
            }

            break;
        }
    }
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
    Arena out = {};
    switch (type) {
        case EArenaType::FixedSize: {
            Arena arena{
                .Size = size,
                .Offset = 0,
                .Type = type,
            };
            arena.Start = (u8*)memory_private::AllocMemory(&arena, size);
            out = std::move(arena);

            break;
        }
        case EArenaType::Extendable: {
            out = memory_private::AllocExtendableArena(size);
            break;
        }
    }

    memory_private::ConsolidateNumbers(&out);
    return out;
}

void FreeArena(Arena* arena) {
    ASSERT(IsValid(*arena));

    switch (arena->Type) {
        case EArenaType::FixedSize: {
            memory_private::FreeMemory(arena, arena->Start);
            break;
        }
        case EArenaType::Extendable: {
            memory_private::FreeExtendableArena(arena);
            break;
        }
    }
}

void ArenaReset(Arena* arena) {
    switch (arena->Type) {
        case EArenaType::FixedSize: {
            arena->Offset = 0;
            break;
        }
        case EArenaType::Extendable: {
            if (Arena* next_arena = arena->ExtendableData.NextArena) {
                arena->Stats.FreeCalls += memory_private::FreeExtendableArena(next_arena);
                arena->ExtendableData.NextArena = nullptr;
            }
            arena->Offset = 0;
            break;
        }
    }

    memory_private::ConsolidateNumbers(arena);
}

u8* ArenaPush(Arena* arena, u64 size, u64 alignment) {
    ASSERT(IsValid(*arena));

    u8* out = nullptr;

    switch (arena->Type) {
        case EArenaType::FixedSize: {
            // Determine the new offset
            u8* ptr = arena->Start + arena->Offset;
            ptr = (u8*)AlignForward(ptr, alignment);
            u64 offset = ptr - arena->Start;

            ASSERT(offset + size < arena->Size);

            arena->Offset = offset + size;
            out = ptr;
            break;
        }
        case EArenaType::Extendable: {
            out = memory_private::ExtendableArenaPush(arena, size, alignment);
            break;
        }
    }

    memory_private::ConsolidateNumbers(arena);
    return out;
}

u8* ArenaPushZero(Arena* arena, u64 size, u64 alignment) {
    u8* ptr = ArenaPush(arena, size, alignment);
    std::memset(ptr, 0, size);
    return ptr;
}

const char* InternString(Arena* arena, const char* string) {
    u64 len = std::strlen(string) + 1;  // Extra byte for the null terminator.
    u8* ptr = ArenaPush(arena, len);
    std::memcpy(ptr, string, len);
    return (const char*)ptr;
}

void* Align(void* ptr, u64 alignment) {
    ASSERT(IsPowerOf2(alignment));

    u64 v = (u64)ptr;
    u64 mask = alignment - 1;

    // Clear the least significant bits up to alignment using bitwise AND with inverted mask.
    v &= ~mask;

    return (void*)v;
}

void* AlignForward(void* ptr, u64 alignment) {
    ASSERT(IsPowerOf2(alignment));

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

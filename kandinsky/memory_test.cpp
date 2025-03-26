#include <catch2/catch_test_macros.hpp>

#include <kandinsky/memory.h>
#include <kandinsky/print.h>

using namespace kdk;

TEST_CASE("Arena - FixedSize", "[memory][arena]") {
    SECTION("Simple allocation") {
        constexpr u64 kAllocSize = 4 * KILOBYTE;
        Arena arena = AllocateArena(kAllocSize);
        REQUIRE(arena.Start != nullptr);
        REQUIRE(arena.Size == kAllocSize);
        REQUIRE(arena.Offset == 0);

        constexpr u64 kPushSize = 16;
        {
            u8* ptr = ArenaPush(&arena, kPushSize);
            REQUIRE(arena.Offset == kPushSize);
            for (u32 i = 0; i < kPushSize; i++) {
                *ptr++ = (u8)i;
            }
        }

        ArenaReset(&arena);
        REQUIRE(arena.Size == kAllocSize);
        REQUIRE(arena.Offset == 0);

        {
            u8* ptr = ArenaPushZero(&arena, kPushSize);
            for (u32 i = 0; i < kPushSize; i++) {
                REQUIRE(*ptr++ == 0);
            }
        }
    }
}

TEST_CASE("Arena - Extendable", "[memory][arena]") {
    SECTION("Allocate single piece") {
        // IMPORTANT: If sizeof(Arena) changes, you need to update this value.
        static_assert(sizeof(Arena) == 64);
        constexpr u64 kMaxLinkOffset = 1024 - 64;

        Arena arena = AllocateArena(1024, EArenaType::Extendable);
        REQUIRE(IsValid(arena));
        REQUIRE(arena.Type == EArenaType::Extendable);
        REQUIRE(arena.Size == 1024);
        REQUIRE(arena.Offset == 0);
        REQUIRE(arena.ExtendableData.NextArena == nullptr);
        REQUIRE(arena.ExtendableData.MaxLinkOffset == kMaxLinkOffset);
        REQUIRE(arena.Stats.AllocCalls == 1);
        REQUIRE(arena.Stats.FreeCalls == 0);

        ArenaPush(&arena, 512);
        REQUIRE(arena.Size == 1024);
        REQUIRE(arena.Offset == 512);
        REQUIRE(arena.ExtendableData.NextArena == nullptr);
        REQUIRE(arena.Stats.AllocCalls == 1);
        REQUIRE(arena.Stats.FreeCalls == 0);

        // Push *just before* the limit.
        ArenaPush(&arena, 448);
        REQUIRE(arena.Size == 1024);
        REQUIRE(arena.Offset == 960);
        REQUIRE(arena.ExtendableData.NextArena == nullptr);
        REQUIRE(arena.Stats.AllocCalls == 1);
        REQUIRE(arena.Stats.FreeCalls == 0);

        // We should now allocate again.
        {
            ArenaPush(&arena, 512);
            REQUIRE(arena.Size == 1024);
            REQUIRE(arena.ExtendableData.TotalSize == 2048);
            REQUIRE(arena.Offset == 960);
            REQUIRE(arena.Stats.AllocCalls == 2);
            REQUIRE(arena.Stats.FreeCalls == 0);

            Arena* next_arena = arena.ExtendableData.NextArena;
            REQUIRE(next_arena);
            REQUIRE(next_arena->Type == EArenaType::Extendable);
            REQUIRE(next_arena->Size == 1024);
            REQUIRE(next_arena->ExtendableData.TotalSize == 0);
            REQUIRE(next_arena->Offset == 512);
            REQUIRE(next_arena->ExtendableData.NextArena == nullptr);
            REQUIRE(next_arena->ExtendableData.MaxLinkOffset == kMaxLinkOffset);
        }

        // And again.
        {
            ArenaPush(&arena, 512);
            REQUIRE(arena.Size == 1024);
            REQUIRE(arena.ExtendableData.TotalSize == 3072);
            REQUIRE(arena.Offset == 960);
            REQUIRE(arena.Stats.AllocCalls == 3);
            REQUIRE(arena.Stats.FreeCalls == 0);

            Arena* next_arena = arena.ExtendableData.NextArena;
            REQUIRE(next_arena);
            REQUIRE(next_arena->Type == EArenaType::Extendable);
            REQUIRE(next_arena->Size == 1024);
            REQUIRE(next_arena->ExtendableData.TotalSize == 0);
            REQUIRE(next_arena->Offset == 512);
            REQUIRE(next_arena->ExtendableData.NextArena);
            REQUIRE(next_arena->ExtendableData.MaxLinkOffset == kMaxLinkOffset);

            next_arena = next_arena->ExtendableData.NextArena;
            REQUIRE(next_arena);
            REQUIRE(next_arena->Type == EArenaType::Extendable);
            REQUIRE(next_arena->ExtendableData.TotalSize == 0);
            REQUIRE(next_arena->Offset == 512);
            REQUIRE(next_arena->ExtendableData.NextArena == nullptr);
            REQUIRE(next_arena->ExtendableData.MaxLinkOffset == kMaxLinkOffset);
        }

        // And again!
        {
            ArenaPush(&arena, 512);
            REQUIRE(arena.Size == 1024);
            REQUIRE(arena.ExtendableData.TotalSize == 4096);
            REQUIRE(arena.Offset == 960);
            REQUIRE(arena.Stats.AllocCalls == 4);
            REQUIRE(arena.Stats.FreeCalls == 0);

            Arena* next_arena = arena.ExtendableData.NextArena;
            REQUIRE(next_arena);
            REQUIRE(next_arena->Type == EArenaType::Extendable);
            REQUIRE(next_arena->Size == 1024);
            REQUIRE(next_arena->ExtendableData.TotalSize == 0);
            REQUIRE(next_arena->Offset == 512);
            REQUIRE(next_arena->ExtendableData.NextArena);
            REQUIRE(next_arena->ExtendableData.MaxLinkOffset == kMaxLinkOffset);

            next_arena = next_arena->ExtendableData.NextArena;
            REQUIRE(next_arena);
            REQUIRE(next_arena->Type == EArenaType::Extendable);
            REQUIRE(next_arena->Size == 1024);
            REQUIRE(next_arena->ExtendableData.TotalSize == 0);
            REQUIRE(next_arena->Offset == 512);
            REQUIRE(next_arena->ExtendableData.NextArena);
            REQUIRE(next_arena->ExtendableData.MaxLinkOffset == kMaxLinkOffset);

            next_arena = next_arena->ExtendableData.NextArena;
            REQUIRE(next_arena);
            REQUIRE(next_arena->Type == EArenaType::Extendable);
            REQUIRE(next_arena->ExtendableData.TotalSize == 0);
            REQUIRE(next_arena->Offset == 512);
            REQUIRE(next_arena->ExtendableData.NextArena == nullptr);
            REQUIRE(next_arena->ExtendableData.MaxLinkOffset == kMaxLinkOffset);
        }

        // Clear everything.
        ArenaReset(&arena);
        REQUIRE(arena.Size == 1024);
        REQUIRE(arena.ExtendableData.TotalSize == 1024);
        REQUIRE(arena.Offset == 0);
        REQUIRE(arena.Stats.AllocCalls == 4);
        REQUIRE(arena.Stats.FreeCalls == 3);
        REQUIRE(arena.ExtendableData.NextArena == nullptr);
    }
}

TEST_CASE("Align", "[memory][Align]") {
    SECTION("Already aligned pointers remain unchanged") {
        void* ptr = (void*)0x1000;  // Aligned to 4096
        REQUIRE(Align(ptr, 16) == ptr);
        REQUIRE(Align(ptr, 32) == ptr);
        REQUIRE(Align(ptr, 4096) == ptr);
    }

    SECTION("Align to different power-of-2 boundaries") {
        void* ptr = (void*)0x1234;
        REQUIRE((u64)Align(ptr, 16) == 0x1230);
        REQUIRE((u64)Align(ptr, 32) == 0x1220);
        REQUIRE((u64)Align(ptr, 64) == 0x1200);
    }

    SECTION("Align null pointer") {
        void* ptr = nullptr;
        REQUIRE(Align(ptr, 16) == ptr);
        REQUIRE(Align(ptr, 32) == ptr);
        REQUIRE(Align(ptr, 4096) == ptr);
    }

    SECTION("Align with different starting addresses") {
        void* ptr1 = (void*)0x1235;
        void* ptr2 = (void*)0x1239;

        // Both should align to 0x1230 with 16-byte alignment
        REQUIRE(Align(ptr1, 16) == Align(ptr2, 16));
        REQUIRE((u64)Align(ptr1, 16) == 0x1230);
    }

    SECTION("Maximum alignment values") {
        void* ptr = (void*)0x1234567890ABCDEF;

        // Test with large power-of-2 alignments
        REQUIRE((u64)Align(ptr, 0x1000) == 0x1234567890ABC000);
        REQUIRE((u64)Align(ptr, 0x10000) == 0x1234567890AB0000);
    }

    SECTION("Address near zero") {
        void* ptr = (void*)15;  // 0xF
        REQUIRE((u64)Align(ptr, 16) == 0x0);
        REQUIRE((u64)Align(ptr, 32) == 0x0);
    }

    SECTION("Address near type limit") {
        void* ptr = (void*)(~0ULL - 10);       // Near maximum value
        u64 expected = (~0ULL - 10) & ~15ULL;  // Manual calculation for 16-byte alignment
        REQUIRE((u64)Align(ptr, 16) == expected);
    }
}

TEST_CASE("AlignForward", "[memory][AlignForward]") {
    SECTION("Already aligned pointers remain unchanged") {
        void* ptr = (void*)0x1000;  // Aligned to 4096
        REQUIRE(AlignForward(ptr, 16) == ptr);
        REQUIRE(AlignForward(ptr, 32) == ptr);
        REQUIRE(AlignForward(ptr, 4096) == ptr);
    }

    SECTION("Align to next power-of-2 boundary") {
        void* ptr = (void*)0x1234;                      // 4660 in decimal
        REQUIRE((u64)AlignForward(ptr, 16) == 0x1240);  // Next 16-byte boundary
        REQUIRE((u64)AlignForward(ptr, 32) == 0x1240);  // Next 32-byte boundary
        REQUIRE((u64)AlignForward(ptr, 64) == 0x1240);  // Next 64-byte boundary
    }

    SECTION("Align null pointer") {
        void* ptr = nullptr;
        REQUIRE(AlignForward(ptr, 16) == ptr);
        REQUIRE(AlignForward(ptr, 32) == ptr);
        REQUIRE(AlignForward(ptr, 4096) == ptr);
    }

    SECTION("Address one below alignment boundary") {
        void* ptr = (void*)0x123F;  // One below 0x1240 (16-byte boundary)
        REQUIRE((u64)AlignForward(ptr, 16) == 0x1240);
    }

    SECTION("Address one above alignment boundary") {
        void* ptr = (void*)0x1241;  // One above 0x1240 (16-byte boundary)
        REQUIRE((u64)AlignForward(ptr, 16) == 0x1250);
    }

    SECTION("Maximum alignment values") {
        void* ptr = (void*)0x1234567890ABCDEF;

        // Test with large power-of-2 alignments
        REQUIRE((u64)AlignForward(ptr, 0x1000) == 0x1234567890ABD000);
        REQUIRE((u64)AlignForward(ptr, 0x10000) == 0x1234567890AC0000);
    }

    SECTION("Address near zero") {
        void* ptr = (void*)1;
        REQUIRE((u64)AlignForward(ptr, 16) == 0x10);
        REQUIRE((u64)AlignForward(ptr, 32) == 0x20);
    }

    SECTION("Address near type limit") {
        // Test case near maximum value, but with enough space to align forward
        void* ptr = (void*)(~0ULL - 20);
        u64 aligned = (u64)AlignForward(ptr, 16);
        REQUIRE(aligned > (u64)ptr);
        REQUIRE((aligned & 15) == 0);  // Verify alignment
    }
}

TEST_CASE("Compare Align and AlignForward behaviors", "[Memory][Align][AlignForward]") {
    SECTION("Compare Align and AlignForward") {
        void* ptr = (void*)0x1234;
        REQUIRE((u64)Align(ptr, 16) == 0x1230);         // Previous 16-byte boundary
        REQUIRE((u64)AlignForward(ptr, 16) == 0x1240);  // Next 16-byte boundary
    }

    SECTION("Alignment differences for various addresses") {
        // Test several addresses with different alignments
        for (u64 addr = 0x1230; addr < 0x1240; addr++) {
            void* ptr = (void*)addr;
            u64 backward = (u64)Align(ptr, 16);
            u64 forward = (u64)AlignForward(ptr, 16);

            // Forward alignment should be >= original address
            REQUIRE(forward >= addr);
            // Backward alignment should be <= original address
            REQUIRE(backward <= addr);

            // Both alignments should be 16-byte aligned
            REQUIRE((forward & 15) == 0);
            REQUIRE((backward & 15) == 0);

            // Forward alignment should be the next 16-byte boundary after backward
            if (addr & 15) {  // If address is not already aligned
                REQUIRE(forward == backward + 16);
            } else {  // If address is already aligned
                REQUIRE(forward == backward);
            }
        }
    }
}

TEST_CASE("IsPowerOf2", "[memory]") {
    SECTION("Valid power of 2 values") {
        REQUIRE(IsPowerOf2(1));
        REQUIRE(IsPowerOf2(2));
        REQUIRE(IsPowerOf2(4));
        REQUIRE(IsPowerOf2(8));
        REQUIRE(IsPowerOf2(16));
        REQUIRE(IsPowerOf2(32));
        REQUIRE(IsPowerOf2(64));
        REQUIRE(IsPowerOf2(128));
        REQUIRE(IsPowerOf2(256));
        REQUIRE(IsPowerOf2(512));
        REQUIRE(IsPowerOf2(1024));
        REQUIRE(IsPowerOf2(0x8000000000000000));
    }

    SECTION("Non-power of 2 values") {
        REQUIRE_FALSE(IsPowerOf2(3));
        REQUIRE_FALSE(IsPowerOf2(6));
        REQUIRE_FALSE(IsPowerOf2(7));
        REQUIRE_FALSE(IsPowerOf2(9));
        REQUIRE_FALSE(IsPowerOf2(10));
        REQUIRE_FALSE(IsPowerOf2(15));
        REQUIRE_FALSE(IsPowerOf2(0xFFFFFFFFFFFFFFFF));
    }
}

namespace memory_test_private {

const char* SomeFile(Arena* arena, int number) { return Printf(arena, "foo_%d", number); }

}  // namespace memory_test_private

TEST_CASE("Scratch arena", "[memory]") {
    SECTION("Can get scratch arena") {
        using namespace memory_test_private;

        {
            ScratchArena scratch = GetScratchArena();
            REQUIRE(scratch.Arena->Offset == 0);

            const char* msg1 = SomeFile(scratch.Arena, 33);
            REQUIRE(strcmp(msg1, "foo_33") == 0);
            REQUIRE(scratch.Arena->Offset == 1024);

            const char* msg2 = SomeFile(scratch.Arena, 88);
            REQUIRE(strcmp(msg2, "foo_88") == 0);
            REQUIRE(scratch.Arena->Offset == 2048);
        }

        auto scratch = GetScratchArena();
        REQUIRE(scratch.Arena->Offset == 0);
    }

    SECTION("Arena conflict") {
        using namespace memory_test_private;

        {
            auto scratch1 = GetScratchArena();
            auto scratch2 = GetScratchArena();
            REQUIRE(scratch1.Arena == scratch2.Arena);
        }

        {
            auto scratch1 = GetScratchArena();
            auto scratch2 = GetScratchArena(scratch1.Arena);
            REQUIRE(scratch1.Arena != scratch2.Arena);
        }

        {
            auto scratch1 = GetScratchArena();
            auto scratch2 = GetScratchArena(scratch1.Arena);
            ScratchArena scratch3 = GetScratchArena(scratch1.Arena, scratch2.Arena);
            REQUIRE(scratch1.Arena != scratch3.Arena);
            REQUIRE(scratch2.Arena != scratch3.Arena);
        }
    }

    SECTION("Multiple functions") {
        auto scratch_arenas = ReferenceScratchArenas();
        auto verify_arenas = [&](u64 offset0, u64 offset1, u64 offset2, u64 offset3) {
            REQUIRE(scratch_arenas[0].Offset == offset0);
            REQUIRE(scratch_arenas[1].Offset == offset1);
            REQUIRE(scratch_arenas[2].Offset == offset2);
            REQUIRE(scratch_arenas[3].Offset == offset3);
        };

        verify_arenas(0, 0, 0, 0);

        auto fn1 = [&](Arena* arena1, Arena* arena2) {
            auto scratch = GetScratchArena(arena1, arena2);
            REQUIRE(scratch.Arena == &scratch_arenas[2]);

            ArenaPush(scratch.Arena, 1024);
            verify_arenas(2048, 1024, 1024, 0);
        };

        auto fn2 = [&](Arena* arena) {
            auto scratch = GetScratchArena(arena);
            REQUIRE(scratch.Arena == &scratch_arenas[0]);

            ArenaPush(scratch.Arena, 1024);
            verify_arenas(2048, 1024, 0, 0);
            fn1(arena, scratch.Arena);
            verify_arenas(2048, 1024, 0, 0);
        };

        auto fn3 = [&](Arena* arena) {
            auto scratch = GetScratchArena(arena);
            REQUIRE(scratch.Arena == &scratch_arenas[1]);

            ArenaPush(scratch.Arena, 1024);
            verify_arenas(1024, 1024, 0, 0);
            fn2(scratch.Arena);
            verify_arenas(1024, 1024, 0, 0);
        };

        {
            auto scratch = GetScratchArena();
            REQUIRE(scratch.Arena == &scratch_arenas[0]);
            ArenaPush(scratch.Arena, 1024);
            verify_arenas(1024, 0, 0, 0);
            fn3(scratch.Arena);
            verify_arenas(1024, 0, 0, 0);
        }
        verify_arenas(0, 0, 0, 0);
    }
}

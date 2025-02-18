#include <catch2/catch_test_macros.hpp>

#include <kandinsky/memory.h>

using namespace kdk;

TEST_CASE("Arena", "Simple Allocation") {
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

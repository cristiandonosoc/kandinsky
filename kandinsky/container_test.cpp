#include <catch2/catch_test_macros.hpp>

#include <kandinsky/container.h>
#include <kandinsky/utils/defer.h>

using namespace kdk;

// Tests start here
TEST_CASE("DynArray basic operations", "[dynarray]") {
    SECTION("Creating a new DynArray") {
        Arena arena = AllocateArena(64 * KILOBYTE);
        DEFER { FreeArena(&arena); };

        auto array = NewDynArray<int>(&arena);
        REQUIRE(array._Arena == &arena);
        REQUIRE(array.Base != nullptr);
        REQUIRE(array.Size == 0);
        REQUIRE(array.Cap == kDynArrayInitialCap);
    }

    SECTION("Pushing elements") {
        Arena arena = AllocateArena(64 * KILOBYTE);
        DEFER { FreeArena(&arena); };

        auto array = NewDynArray<int>(&arena);

        auto& ref1 = array.Push(42);
        REQUIRE(array.Size == 1);
        REQUIRE(array.Cap == kDynArrayInitialCap);
        REQUIRE(ref1 == 42);

        auto& ref2 = array.Push(100);
        REQUIRE(array.Size == 2);
        REQUIRE(ref2 == 100);

        REQUIRE(array[0] == 42);
        REQUIRE(array[1] == 100);
    }

    SECTION("Popping elements") {
        Arena arena = AllocateArena(64 * KILOBYTE);
        DEFER { FreeArena(&arena); };

        auto array = NewDynArray<int>(&arena);
        array.Push(10);
        array.Push(20);
        array.Push(30);

        REQUIRE(array.Size == 3);

        int val = array.Pop();
        REQUIRE(val == 30);
        REQUIRE(array.Size == 2);

        val = array.Pop();
        REQUIRE(val == 20);
        REQUIRE(array.Size == 1);

        val = array.Pop();
        REQUIRE(val == 10);
        REQUIRE(array.Size == 0);
    }

    SECTION("Popping from empty array") {
        Arena arena = AllocateArena(64 * KILOBYTE);
        DEFER { FreeArena(&arena); };

        auto array = NewDynArray<int>(&arena);

        int val = array.Pop();
        REQUIRE(val == 0);  // Default-constructed int
    }

    SECTION("Subscript operator access") {
        Arena arena = AllocateArena(64 * KILOBYTE);
        DEFER { FreeArena(&arena); };

        auto array = NewDynArray<int>(&arena);

        array.Push(5);
        array.Push(10);

        REQUIRE(array[0] == 5);
        REQUIRE(array[1] == 10);

        // Modify through reference
        array[0] = 50;
        REQUIRE(array[0] == 50);
    }
}

TEST_CASE("DynArray with non-trivial types", "[dynarray]") {
    SECTION("Using strings") {
        Arena arena = AllocateArena(64 * KILOBYTE);
        DEFER { FreeArena(&arena); };

        auto array = NewDynArray<std::string>(&arena);

        array.Push("Hello");
        array.Push("World");

        REQUIRE(array.Size == 2);
        REQUIRE(array[0] == "Hello");
        REQUIRE(array[1] == "World");

        std::string popped = array.Pop();
        REQUIRE(popped == "World");
        REQUIRE(array.Size == 1);
    }
}

TEST_CASE("DynArray capacity expansion", "[dynarray]") {
    SECTION("Initial capacity") {
        Arena arena = AllocateArena(64 * KILOBYTE);

        auto array = NewDynArray<int>(&arena);

        REQUIRE(array.Cap == kDynArrayInitialCap);  // Should allocate initial capacity
        REQUIRE(arena.Offset == 4 * sizeof(int));

        // Fill in just until capacity.
        u32 cap = array.Cap;
        for (u32 i = 0; i < cap; i++) {
            array.Push(i);
            REQUIRE(array.Size == i + 1);
            REQUIRE(array.Cap == cap);
        }

        // When we hit capacity, it should double.
        array.Push(1001);
        REQUIRE(array.Cap == 2 * kDynArrayInitialCap);
        REQUIRE(array.Size == cap + 1);
        REQUIRE(array.Last() == 1001);

        u32 size = array.Size;
        cap = array.Cap;
        for (u32 i = size; i < cap; i++) {
            array.Push(i);
            REQUIRE(array.Cap == 2 * kDynArrayInitialCap);
            REQUIRE(array.Size == i + 1);
        }

        // Adding one more should double.
        array.Push(2001);
        REQUIRE(array.Cap == 4 * kDynArrayInitialCap);
        REQUIRE(array.Size == cap + 1);
        REQUIRE(array.Last() == 2001);
    }
}

TEST_CASE("DynArray const operations", "[dynarray]") {
    SECTION("Const array access") {
        Arena arena = AllocateArena(64 * KILOBYTE);
        auto array = NewDynArray<int>(&arena);

        array.Push(10);
        array.Push(20);

        const DynArray<int>& constArray = array;

        REQUIRE(constArray[0] == 10);
        REQUIRE(constArray[1] == 20);

        // Cannot modify through const reference
        // constArray.Push(30);  // Should not compile
    }
}

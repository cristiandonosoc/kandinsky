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

#if 0

TEST_CASE("DynArray capacity expansion", "[dynarray]") {
    SECTION("Initial capacity") {
        Arena arena = AllocateArena(64 * KILOBYTE);

        auto array = NewDynArray<int>(&arena);

        array.Push(1);
        REQUIRE(array.Cap > 0);  // Should allocate initial capacity

        u32 initialCap = 0;

        // Fill to capacity
        for (int i = 0; i < 10; i++) {
            array.Push(i);

            if (i == 0) {
                initialCap = array.Cap;
                REQUIRE(initialCap > 0);
            }

            // When we hit capacity, it should double
            if (array.Size > initialCap && initialCap > 0) {
                REQUIRE(array.Cap >= initialCap * 2);
                break;
            }
        }

        const int COUNT = 100;

        for (int i = 0; i < COUNT; i++) {
            array.Push(i);
        }

        REQUIRE(array.Size == COUNT);

        // Check all elements were stored correctly
        for (int i = 0; i < COUNT; i++) {
            REQUIRE(array[i] == i);
        }
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

// This test would fail with the original implementation
TEST_CASE("DynArray assertion tests", "[dynarray][!shouldfail]") {
    SECTION("Out of bounds access") {
        Arena arena = AllocateArena(64 * KILOBYTE);
        auto array = NewDynArray<int>(&arena);
        array.Push(10);

        // This should trigger the ASSERT and throw
        REQUIRE_THROWS(array[1]);
    }
}
#endif

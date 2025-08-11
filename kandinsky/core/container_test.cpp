#include <catch2/catch_test_macros.hpp>

#include <kandinsky/core/container.h>
#include <kandinsky/core/memory.h>

using namespace kdk;

// Tests start here
TEST_CASE("DynArray basic operations", "[dynarray]") {
    SECTION("Default initialized DynArray") {
        DynArray<int> array = {};
        REQUIRE(array.Base == nullptr);
        REQUIRE(array.Size == 0);
        REQUIRE(array.Cap == 0);

        Arena arena = AllocateArena(64 * KILOBYTE);
        DEFER { FreeArena(&arena); };

        // First push should initialize the array
        array.Push(&arena, 42);
        REQUIRE(array.Base != nullptr);
        REQUIRE(array.Size == 1);
        REQUIRE(array.Cap == kDynArrayInitialCap);
        REQUIRE(array[0] == 42);
    }

    SECTION("Creating a new DynArray") {
        Arena arena = AllocateArena(64 * KILOBYTE);
        DEFER { FreeArena(&arena); };

        auto array = NewDynArray<int>(&arena);
        REQUIRE(array.Base != nullptr);
        REQUIRE(array.Size == 0);
        REQUIRE(array.Cap == kDynArrayInitialCap);
    }

    SECTION("Pushing elements") {
        Arena arena = AllocateArena(64 * KILOBYTE);
        DEFER { FreeArena(&arena); };

        auto array = NewDynArray<int>(&arena);

        auto& ref1 = array.Push(&arena, 42);
        REQUIRE(array.Size == 1);
        REQUIRE(array.Cap == kDynArrayInitialCap);
        REQUIRE(ref1 == 42);

        auto& ref2 = array.Push(&arena, 100);
        REQUIRE(array.Size == 2);
        REQUIRE(ref2 == 100);

        REQUIRE(array[0] == 42);
        REQUIRE(array[1] == 100);
    }

    SECTION("Popping elements") {
        Arena arena = AllocateArena(64 * KILOBYTE);
        DEFER { FreeArena(&arena); };

        auto array = NewDynArray<int>(&arena);
        array.Push(&arena, 10);
        array.Push(&arena, 20);
        array.Push(&arena, 30);

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

        array.Push(&arena, 5);
        array.Push(&arena, 10);

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

        array.Push(&arena, "Hello");
        array.Push(&arena, "World");

        REQUIRE(array.Size == 2);
        REQUIRE(array[0] == "Hello");
        REQUIRE(array[1] == "World");

        std::string popped = array.Pop();
        REQUIRE(popped == "World");
        REQUIRE(array.Size == 1);
    }

    SECTION("Capacity expansion with non-trivial type") {
        Arena arena = AllocateArena(64 * KILOBYTE);
        DEFER { FreeArena(&arena); };

        auto array = NewDynArray<std::string>(&arena);

        // Fill beyond initial capacity to test reallocation
        for (int i = 0; i < 10; i++) {
            array.Push(&arena, "String " + std::to_string(i));
        }

        REQUIRE(array.Size == 10);
        REQUIRE(array.Cap >= 10);

        // Verify all strings are intact after reallocation
        for (u32 i = 0; i < array.Size; i++) {
            REQUIRE(array[i] == "String " + std::to_string(i));
        }
    }

    SECTION("Move semantics") {
        Arena arena = AllocateArena(64 * KILOBYTE);
        DEFER { FreeArena(&arena); };

        struct MoveOnly {
            std::string data;
            bool moved = false;

            MoveOnly() = default;
            MoveOnly(const std::string& s) : data(s) {}
            MoveOnly(const MoveOnly& other) = default;
            MoveOnly& operator=(const MoveOnly&) = delete;

            MoveOnly(MoveOnly&& other) noexcept : data(std::move(other.data)) {
                other.moved = true;
            }

            MoveOnly& operator=(MoveOnly&& other) noexcept {
                data = std::move(other.data);
                other.moved = true;
                return *this;
            }
        };

        auto array = NewDynArray<MoveOnly>(&arena);

        array.Push(&arena, MoveOnly("test1"));
        array.Push(&arena, MoveOnly("test2"));

        REQUIRE(array.Size == 2);
        REQUIRE(array[0].data == "test1");
        REQUIRE(array[1].data == "test2");

        MoveOnly popped = array.Pop();
        REQUIRE(popped.data == "test2");
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
            array.Push(&arena, i);
            REQUIRE(array.Size == i + 1);
            REQUIRE(array.Cap == cap);
        }

        // When we hit capacity, it should double.
        array.Push(&arena, 1001);
        REQUIRE(array.Cap == 2 * kDynArrayInitialCap);
        REQUIRE(array.Size == cap + 1);
        REQUIRE(array.Last() == 1001);

        u32 size = array.Size;
        cap = array.Cap;
        for (u32 i = size; i < cap; i++) {
            array.Push(&arena, i);
            REQUIRE(array.Cap == 2 * kDynArrayInitialCap);
            REQUIRE(array.Size == i + 1);
        }

        // Adding one more should double.
        array.Push(&arena, 2001);
        REQUIRE(array.Cap == 4 * kDynArrayInitialCap);
        REQUIRE(array.Size == cap + 1);
        REQUIRE(array.Last() == 2001);
    }
}

TEST_CASE("DynArray Reserve operations", "[dynarray]") {
    SECTION("Reserve with empty array") {
        Arena arena = AllocateArena(64 * KILOBYTE);
        DEFER { FreeArena(&arena); };

        auto array = NewDynArray<int>(&arena);
        REQUIRE(array.Cap == kDynArrayInitialCap);

        array.Reserve(&arena, 16);
        REQUIRE(array.Cap == 16);
        REQUIRE(array.Size == 0);
        REQUIRE(array.Base != nullptr);
    }

    SECTION("Reserve with existing elements") {
        Arena arena = AllocateArena(64 * KILOBYTE);
        DEFER { FreeArena(&arena); };

        auto array = NewDynArray<int>(&arena);

        // Add some elements
        array.Push(&arena, 1);
        array.Push(&arena, 2);
        array.Push(&arena, 3);

        u32 originalSize = array.Size;
        array.Reserve(&arena, 20);

        REQUIRE(array.Cap == 20);
        REQUIRE(array.Size == originalSize);
        // Verify elements are preserved
        REQUIRE(array[0] == 1);
        REQUIRE(array[1] == 2);
        REQUIRE(array[2] == 3);
    }

    SECTION("Reserve with non-trivial type") {
        Arena arena = AllocateArena(64 * KILOBYTE);
        DEFER { FreeArena(&arena); };

        auto array = NewDynArray<std::string>(&arena);

        array.Push(&arena, "test1");
        array.Push(&arena, "test2");

        array.Reserve(&arena, 10);
        REQUIRE(array.Cap == 10);
        REQUIRE(array.Size == 2);
        REQUIRE(array[0] == "test1");
        REQUIRE(array[1] == "test2");
    }

    SECTION("Reserve smaller than current capacity") {
        Arena arena = AllocateArena(64 * KILOBYTE);
        DEFER { FreeArena(&arena); };

        auto array = NewDynArray<int>(&arena, 8);
        u32 originalCap = array.Cap;

        array.Reserve(&arena, 4);           // Try to reserve less than current capacity
        REQUIRE(array.Cap == originalCap);  // Should not shrink
    }
}

TEST_CASE("FixedArray Remove operations", "[fixedarray]") {
    SECTION("Remove single element") {
        FixedArray<int, 8> array;
        array.Push(1);
        array.Push(2);
        array.Push(2);
        array.Push(3);
        array.Push(2);
        array.Push(4);
        REQUIRE(array.Size == 6);

        // Remove first occurrence of 2
        i32 removed = array.Remove(2);
        REQUIRE(removed == 1);
        REQUIRE(array.Size == 5);
        REQUIRE(array[0] == 1);
        REQUIRE(array[1] == 2);  // Second 2 remains
        REQUIRE(array[2] == 3);
        REQUIRE(array[3] == 2);  // Third 2 remains
        REQUIRE(array[4] == 4);
    }

    SECTION("Remove multiple elements") {
        FixedArray<int, 8> array;
        array.Push(1);
        array.Push(2);
        array.Push(2);
        array.Push(3);
        array.Push(2);
        array.Push(4);

        // Remove two occurrences of 2
        i32 removed = array.Remove(2, 2);
        REQUIRE(removed == 2);
        REQUIRE(array.Size == 4);
        REQUIRE(array[0] == 1);
        REQUIRE(array[1] == 3);
        REQUIRE(array[2] == 2);  // Last 2 remains
        REQUIRE(array[3] == 4);
    }

    SECTION("RemoveAll elements") {
        FixedArray<int, 8> array;
        array.Push(1);
        array.Push(2);
        array.Push(2);
        array.Push(3);
        array.Push(2);
        array.Push(4);

        // Remove all occurrences of 2
        i32 removed = array.RemoveAll(2);
        REQUIRE(removed == 3);
        REQUIRE(array.Size == 3);
        REQUIRE(array[0] == 1);
        REQUIRE(array[1] == 3);
        REQUIRE(array[2] == 4);
    }

    SECTION("RemoveAll elements, which empties array.") {
        FixedArray<int, 8> array;
        array.Push(2);
        array.Push(2);
        array.Push(2);
        array.Push(2);
        array.Push(2);
        array.Push(2);
        REQUIRE(array.Size == 6);

        // Remove all occurrences of 2
        i32 removed = array.RemoveAll(2);
        REQUIRE(removed == 6);
        REQUIRE(array.Size == 0);
    }

    SECTION("Remove from empty array") {
        FixedArray<int, 4> array;
        i32 removed = array.Remove(1);
        REQUIRE(removed == 0);
        REQUIRE(array.Size == 0);
    }

    SECTION("Remove non-existent element") {
        FixedArray<int, 4> array;
        array.Push(1);
        array.Push(2);
        array.Push(3);

        i32 removed = array.Remove(4);
        REQUIRE(removed == 0);
        REQUIRE(array.Size == 3);
        REQUIRE(array[0] == 1);
        REQUIRE(array[1] == 2);
        REQUIRE(array[2] == 3);
    }

    SECTION("Remove with non-POD type") {
        FixedArray<std::string, 8> array;
        array.Push("hello");
        array.Push("world");
        array.Push("hello");
        array.Push("test");
        array.Push("hello");

        // Remove two occurrences of "hello"
        i32 removed = array.Remove("hello", 2);
        REQUIRE(removed == 2);
        REQUIRE(array.Size == 3);
        REQUIRE(array[0] == "world");
        REQUIRE(array[1] == "test");
        REQUIRE(array[2] == "hello");  // Last "hello" remains

        // Remove remaining "hello"
        removed = array.Remove("hello");
        REQUIRE(removed == 1);
        REQUIRE(array.Size == 2);
        REQUIRE(array[0] == "world");
        REQUIRE(array[1] == "test");
    }

    SECTION("Remove with move-only type") {
        struct MoveOnly {
            std::string data;
            bool moved = false;

            MoveOnly() = default;
            MoveOnly(const std::string& s) : data(s) {}
            MoveOnly(const MoveOnly&) = default;
            MoveOnly& operator=(const MoveOnly&) = delete;

            MoveOnly(MoveOnly&& other) noexcept : data(std::move(other.data)) {
                other.moved = true;
            }

            MoveOnly& operator=(MoveOnly&& other) noexcept {
                data = std::move(other.data);
                other.moved = true;
                return *this;
            }

            bool operator==(const MoveOnly& other) const { return data == other.data; }
        };

        FixedArray<MoveOnly, 8> array;
        array.Push(MoveOnly("one"));
        array.Push(MoveOnly("two"));
        array.Push(MoveOnly("two"));
        array.Push(MoveOnly("three"));

        // Remove one occurrence of "two"
        i32 removed = array.Remove(MoveOnly("two"));
        REQUIRE(removed == 1);
        REQUIRE(array.Size == 3);
        REQUIRE(array[0].data == "one");
        REQUIRE(array[1].data == "two");
        REQUIRE(array[2].data == "three");
    }

    SECTION("RemovePred with basic predicate") {
        FixedArray<int, 8> array;
        array.Push(1);
        array.Push(2);
        array.Push(3);
        array.Push(4);
        array.Push(5);
        array.Push(6);

        // Remove even numbers (up to 2)
        i32 removed = array.RemovePred([](const int& x) { return x % 2 == 0; }, 2);
        REQUIRE(removed == 2);
        REQUIRE(array.Size == 4);
        REQUIRE(array[0] == 1);
        REQUIRE(array[1] == 3);
        REQUIRE(array[2] == 5);
        REQUIRE(array[3] == 6);  // Last even number remains

        // Remove remaining even numbers
        removed = array.RemovePred([](const int& x) { return x % 2 == 0; });
        REQUIRE(removed == 1);
        REQUIRE(array.Size == 3);
        REQUIRE(array[0] == 1);
        REQUIRE(array[1] == 3);
        REQUIRE(array[2] == 5);
    }

    SECTION("RemovePred with string predicate") {
        FixedArray<std::string, 8> array;
        array.Push("hello");
        array.Push("world");
        array.Push("test123");
        array.Push("hello123");
        array.Push("testing");

        // Remove strings containing "123"
        i32 removed = array.RemovePred(
            [](const std::string& s) { return s.find("123") != std::string::npos; },
            2);
        REQUIRE(removed == 2);
        REQUIRE(array.Size == 3);
        REQUIRE(array[0] == "hello");
        REQUIRE(array[1] == "world");
        REQUIRE(array[2] == "testing");
    }

    SECTION("RemovePred with empty array") {
        FixedArray<int, 4> array;
        i32 removed = array.RemovePred([](const int& x) { return x > 0; });
        REQUIRE(removed == 0);
        REQUIRE(array.Size == 0);
    }

    SECTION("RemovePred with non-matching predicate") {
        FixedArray<int, 4> array;
        array.Push(1);
        array.Push(2);
        array.Push(3);

        i32 removed = array.RemovePred([](const int& x) { return x > 10; });
        REQUIRE(removed == 0);
        REQUIRE(array.Size == 3);
        REQUIRE(array[0] == 1);
        REQUIRE(array[1] == 2);
        REQUIRE(array[2] == 3);
    }

    SECTION("RemovePred with move-only type") {
        struct MoveOnly {
            int value = 0;
            bool moved = false;

            MoveOnly() = default;
            MoveOnly(int v) : value(v) {}
            MoveOnly(const MoveOnly&) = default;
            MoveOnly& operator=(const MoveOnly&) = delete;

            MoveOnly(MoveOnly&& other) noexcept : value(other.value) { other.moved = true; }

            MoveOnly& operator=(MoveOnly&& other) noexcept {
                value = other.value;
                other.moved = true;
                return *this;
            }
        };

        FixedArray<MoveOnly, 8> array;
        array.Push(MoveOnly(1));
        array.Push(MoveOnly(2));
        array.Push(MoveOnly(3));
        array.Push(MoveOnly(4));

        // Remove even numbers
        i32 removed = array.RemoveAllPred([](const MoveOnly& x) { return x.value % 2 == 0; });
        REQUIRE(removed == 2);
        REQUIRE(array.Size == 2);
        REQUIRE(array[0].value == 1);
        REQUIRE(array[1].value == 3);
    }
}

TEST_CASE("DynArray const operations", "[dynarray]") {
    SECTION("Const array access") {
        Arena arena = AllocateArena(64 * KILOBYTE);
        auto array = NewDynArray<int>(&arena);

        array.Push(&arena, 10);
        array.Push(&arena, 20);

        const DynArray<int>& constArray = array;

        REQUIRE(constArray[0] == 10);
        REQUIRE(constArray[1] == 20);

        // Cannot modify through const reference
        // constArray.Push(30);  // Should not compile
    }
}

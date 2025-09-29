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
        for (i32 i = 0; i < array.Size; i++) {
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

TEST_CASE("FixedVector Remove operations", "[FixedVector]") {
    SECTION("Remove single element") {
        FixedVector<int, 8> vector;
        vector.Push(1);
        vector.Push(2);
        vector.Push(2);
        vector.Push(3);
        vector.Push(2);
        vector.Push(4);
        REQUIRE(vector.Size == 6);

        // Remove first occurrence of 2
        i32 removed = vector.Remove(2);
        REQUIRE(removed == 1);
        REQUIRE(vector.Size == 5);
        REQUIRE(vector[0] == 1);
        REQUIRE(vector[1] == 2);  // Second 2 remains
        REQUIRE(vector[2] == 3);
        REQUIRE(vector[3] == 2);  // Third 2 remains
        REQUIRE(vector[4] == 4);
    }

    SECTION("Remove multiple elements") {
        FixedVector<int, 8> vector;
        vector.Push(1);
        vector.Push(2);
        vector.Push(2);
        vector.Push(3);
        vector.Push(2);
        vector.Push(4);

        // Remove two occurrences of 2
        i32 removed = vector.Remove(2, 2);
        REQUIRE(removed == 2);
        REQUIRE(vector.Size == 4);
        REQUIRE(vector[0] == 1);
        REQUIRE(vector[1] == 3);
        REQUIRE(vector[2] == 2);  // Last 2 remains
        REQUIRE(vector[3] == 4);
    }

    SECTION("RemoveAll elements") {
        FixedVector<int, 8> vector;
        vector.Push(1);
        vector.Push(2);
        vector.Push(2);
        vector.Push(3);
        vector.Push(2);
        vector.Push(4);

        // Remove all occurrences of 2
        i32 removed = vector.RemoveAll(2);
        REQUIRE(removed == 3);
        REQUIRE(vector.Size == 3);
        REQUIRE(vector[0] == 1);
        REQUIRE(vector[1] == 3);
        REQUIRE(vector[2] == 4);
    }

    SECTION("RemoveAll elements, which empties vector.") {
        FixedVector<int, 8> vector;
        vector.Push(2);
        vector.Push(2);
        vector.Push(2);
        vector.Push(2);
        vector.Push(2);
        vector.Push(2);
        REQUIRE(vector.Size == 6);

        // Remove all occurrences of 2
        i32 removed = vector.RemoveAll(2);
        REQUIRE(removed == 6);
        REQUIRE(vector.Size == 0);
    }

    SECTION("Remove from empty vector") {
        FixedVector<int, 4> vector;
        i32 removed = vector.Remove(1);
        REQUIRE(removed == 0);
        REQUIRE(vector.Size == 0);
    }

    SECTION("Remove non-existent element") {
        FixedVector<int, 4> vector;
        vector.Push(1);
        vector.Push(2);
        vector.Push(3);

        i32 removed = vector.Remove(4);
        REQUIRE(removed == 0);
        REQUIRE(vector.Size == 3);
        REQUIRE(vector[0] == 1);
        REQUIRE(vector[1] == 2);
        REQUIRE(vector[2] == 3);
    }

    SECTION("Remove with non-POD type") {
        FixedVector<std::string, 8> vector;
        vector.Push("hello");
        vector.Push("world");
        vector.Push("hello");
        vector.Push("test");
        vector.Push("hello");

        // Remove two occurrences of "hello"
        i32 removed = vector.Remove("hello", 2);
        REQUIRE(removed == 2);
        REQUIRE(vector.Size == 3);
        REQUIRE(vector[0] == "world");
        REQUIRE(vector[1] == "test");
        REQUIRE(vector[2] == "hello");  // Last "hello" remains

        // Remove remaining "hello"
        removed = vector.Remove("hello");
        REQUIRE(removed == 1);
        REQUIRE(vector.Size == 2);
        REQUIRE(vector[0] == "world");
        REQUIRE(vector[1] == "test");
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

        FixedVector<MoveOnly, 8> vector;
        vector.Push(MoveOnly("one"));
        vector.Push(MoveOnly("two"));
        vector.Push(MoveOnly("two"));
        vector.Push(MoveOnly("three"));

        // Remove one occurrence of "two"
        i32 removed = vector.Remove(MoveOnly("two"));
        REQUIRE(removed == 1);
        REQUIRE(vector.Size == 3);
        REQUIRE(vector[0].data == "one");
        REQUIRE(vector[1].data == "two");
        REQUIRE(vector[2].data == "three");
    }

    SECTION("RemovePred with basic predicate") {
        FixedVector<int, 8> vector;
        vector.Push(1);
        vector.Push(2);
        vector.Push(3);
        vector.Push(4);
        vector.Push(5);
        vector.Push(6);

        // Remove even numbers (up to 2)
        i32 removed = vector.RemovePred([](const int& x) { return x % 2 == 0; }, 2);
        REQUIRE(removed == 2);
        REQUIRE(vector.Size == 4);
        REQUIRE(vector[0] == 1);
        REQUIRE(vector[1] == 3);
        REQUIRE(vector[2] == 5);
        REQUIRE(vector[3] == 6);  // Last even number remains

        // Remove remaining even numbers
        removed = vector.RemovePred([](const int& x) { return x % 2 == 0; });
        REQUIRE(removed == 1);
        REQUIRE(vector.Size == 3);
        REQUIRE(vector[0] == 1);
        REQUIRE(vector[1] == 3);
        REQUIRE(vector[2] == 5);
    }

    SECTION("RemovePred with string predicate") {
        FixedVector<std::string, 8> vector;
        vector.Push("hello");
        vector.Push("world");
        vector.Push("test123");
        vector.Push("hello123");
        vector.Push("testing");

        // Remove strings containing "123"
        i32 removed = vector.RemovePred(
            [](const std::string& s) { return s.find("123") != std::string::npos; },
            2);
        REQUIRE(removed == 2);
        REQUIRE(vector.Size == 3);
        REQUIRE(vector[0] == "hello");
        REQUIRE(vector[1] == "world");
        REQUIRE(vector[2] == "testing");
    }

    SECTION("RemovePred with empty vector") {
        FixedVector<int, 4> vector;
        i32 removed = vector.RemovePred([](const int& x) { return x > 0; });
        REQUIRE(removed == 0);
        REQUIRE(vector.Size == 0);
    }

    SECTION("RemovePred with non-matching predicate") {
        FixedVector<int, 4> vector;
        vector.Push(1);
        vector.Push(2);
        vector.Push(3);

        i32 removed = vector.RemovePred([](const int& x) { return x > 10; });
        REQUIRE(removed == 0);
        REQUIRE(vector.Size == 3);
        REQUIRE(vector[0] == 1);
        REQUIRE(vector[1] == 2);
        REQUIRE(vector[2] == 3);
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

        FixedVector<MoveOnly, 8> vector;
        vector.Push(MoveOnly(1));
        vector.Push(MoveOnly(2));
        vector.Push(MoveOnly(3));
        vector.Push(MoveOnly(4));

        // Remove even numbers
        i32 removed = vector.RemoveAllPred([](const MoveOnly& x) { return x.value % 2 == 0; });
        REQUIRE(removed == 2);
        REQUIRE(vector.Size == 2);
        REQUIRE(vector[0].value == 1);
        REQUIRE(vector[1].value == 3);
    }
}

TEST_CASE("FixedVector RemoveUnordered operations", "[FixedVector]") {
    SECTION("Remove existing element from middle") {
        FixedVector<int, 10> arr;
        arr.Push(1);
        arr.Push(2);
        arr.Push(3);
        arr.Push(4);

        bool result = arr.RemoveUnordered(2);

        REQUIRE(result == true);
        REQUIRE(arr.Size == 3);

        // Element 2 should be gone, but order may have changed
        // The last element (4) should have moved to where 2 was
        bool found_2 = false;
        for (int i = 0; i < arr.Size; ++i) {
            if (arr[i] == 2) {
                found_2 = true;
                break;
            }
        }
        REQUIRE_FALSE(found_2);

        // Should still contain 1, 3, and 4
        REQUIRE(arr.Contains(1));
        REQUIRE(arr.Contains(3));
        REQUIRE(arr.Contains(4));
    }

    SECTION("Remove first element") {
        FixedVector<int, 10> arr;
        arr.Push(10);
        arr.Push(20);
        arr.Push(30);

        bool result = arr.RemoveUnordered(10);

        REQUIRE(result == true);
        REQUIRE(arr.Size == 2);
        REQUIRE_FALSE(arr.Contains(10));
        REQUIRE(arr.Contains(20));
        REQUIRE(arr.Contains(30));
    }

    SECTION("Remove last element") {
        FixedVector<int, 10> arr;
        arr.Push(5);
        arr.Push(15);
        arr.Push(25);

        bool result = arr.RemoveUnordered(25);

        REQUIRE(result == true);
        REQUIRE(arr.Size == 2);
        REQUIRE(arr.Contains(5));
        REQUIRE(arr.Contains(15));
        REQUIRE_FALSE(arr.Contains(25));
    }

    SECTION("Remove non-existent element") {
        FixedVector<int, 10> arr;
        arr.Push(1);
        arr.Push(2);
        arr.Push(3);

        bool result = arr.RemoveUnordered(99);

        REQUIRE(result == false);
        REQUIRE(arr.Size == 3);
        REQUIRE(arr.Contains(1));
        REQUIRE(arr.Contains(2));
        REQUIRE(arr.Contains(3));
    }

    SECTION("Remove from single element array") {
        FixedVector<int, 10> arr;
        arr.Push(42);

        bool result = arr.RemoveUnordered(42);

        REQUIRE(result == true);
        REQUIRE(arr.Size == 0);
        REQUIRE(arr.IsEmpty());
    }

    SECTION("Remove from empty array") {
        FixedVector<int, 10> arr;

        bool result = arr.RemoveUnordered(1);

        REQUIRE(result == false);
        REQUIRE(arr.Size == 0);
        REQUIRE(arr.IsEmpty());
    }

    SECTION("Remove duplicate elements - only removes first found") {
        FixedVector<int, 10> arr;
        arr.Push(1);
        arr.Push(2);
        arr.Push(2);
        arr.Push(3);

        bool result = arr.RemoveUnordered(2);

        REQUIRE(result == true);
        REQUIRE(arr.Size == 3);

        // Should still contain one instance of 2
        int count_2 = 0;
        for (int i = 0; i < arr.Size; ++i) {
            if (arr[i] == 2) {
                count_2++;
            }
        }
        REQUIRE(count_2 == 1);
    }
}

TEST_CASE("FixedVector::RemoveUnorderedPred", "[FixedVector]") {
    SECTION("Remove element matching predicate") {
        FixedVector<int, 10> arr;
        arr.Push(1);
        arr.Push(2);
        arr.Push(3);
        arr.Push(4);
        arr.Push(5);

        // Remove first even number
        auto isEven = [](const int& x) {
            return x % 2 == 0;
        };
        bool result = arr.RemoveUnorderedPred(isEven);

        REQUIRE(result == true);
        REQUIRE(arr.Size == 4);

        // Should have removed either 2 or 4 (whichever was found first)
        bool has_2 = arr.Contains(2);
        bool has_4 = arr.Contains(4);
        REQUIRE((has_2 ^ has_4));  // Exactly one should be true (XOR)

        // Odd numbers should still be present
        REQUIRE(arr.Contains(1));
        REQUIRE(arr.Contains(3));
        REQUIRE(arr.Contains(5));
    }

    SECTION("Remove with predicate that matches no elements") {
        FixedVector<int, 10> arr;
        arr.Push(1);
        arr.Push(3);
        arr.Push(5);

        auto isEven = [](const int& x) {
            return x % 2 == 0;
        };
        bool result = arr.RemoveUnorderedPred(isEven);

        REQUIRE(result == false);
        REQUIRE(arr.Size == 3);
        REQUIRE(arr.Contains(1));
        REQUIRE(arr.Contains(3));
        REQUIRE(arr.Contains(5));
    }

    SECTION("Remove with predicate that matches all elements") {
        FixedVector<int, 10> arr;
        arr.Push(2);
        arr.Push(4);
        arr.Push(6);

        auto isEven = [](const int& x) {
            return x % 2 == 0;
        };
        bool result = arr.RemoveUnorderedPred(isEven);

        REQUIRE(result == true);
        REQUIRE(arr.Size == 2);  // Only removes first match

        // Should still have two even numbers
        int evenCount = 0;
        for (int i = 0; i < arr.Size; ++i) {
            if (arr[i] % 2 == 0) {
                evenCount++;
            }
        }
        REQUIRE(evenCount == 2);
    }

    SECTION("Remove from single element array with matching predicate") {
        FixedVector<int, 10> arr;
        arr.Push(10);

        auto isPositive = [](const int& x) {
            return x > 0;
        };
        bool result = arr.RemoveUnorderedPred(isPositive);

        REQUIRE(result == true);
        REQUIRE(arr.Size == 0);
        REQUIRE(arr.IsEmpty());
    }

    SECTION("Remove from empty array") {
        FixedVector<int, 10> arr;

        auto always_true = [](const int&) {
            return true;
        };
        bool result = arr.RemoveUnorderedPred(always_true);

        REQUIRE(result == false);
        REQUIRE(arr.Size == 0);
        REQUIRE(arr.IsEmpty());
    }

    SECTION("Complex predicate test") {
        FixedVector<std::string, 10> arr;
        arr.Push("hello");
        arr.Push("world");
        arr.Push("test");
        arr.Push("case");

        // Remove first string with length > 4
        auto long_string = [](const std::string& s) {
            return s.length() > 4;
        };
        bool result = arr.RemoveUnorderedPred(long_string);

        REQUIRE(result == true);
        REQUIRE(arr.Size == 3);

        // Either "hello" or "world" should be removed (both have length > 4)
        bool hasHello = arr.Contains("hello");
        bool hasWorld = arr.Contains("world");
        REQUIRE((hasHello ^ hasWorld));  // Exactly one should remain

        REQUIRE(arr.Contains("test"));
        REQUIRE(arr.Contains("case"));
    }
}

TEST_CASE("FixedVector::RemoveUnorderedAt", "[FixedVector]") {
    SECTION("Remove element at valid index") {
        FixedVector<int, 10> arr;
        arr.Push(10);
        arr.Push(20);
        arr.Push(30);
        arr.Push(40);

        arr.RemoveUnorderedAt(1);  // Remove element at index 1 (value 20)

        REQUIRE(arr.Size == 3);

        // Element at index 1 should no longer be 20
        // The last element (40) should have moved to index 1
        REQUIRE_FALSE(arr.Contains(20));
        REQUIRE(arr.Contains(10));
        REQUIRE(arr.Contains(30));
        REQUIRE(arr.Contains(40));
    }

    SECTION("Remove first element") {
        FixedVector<int, 10> arr;
        arr.Push(100);
        arr.Push(200);
        arr.Push(300);

        arr.RemoveUnorderedAt(0);

        REQUIRE(arr.Size == 2);
        REQUIRE_FALSE(arr.Contains(100));
        REQUIRE(arr.Contains(200));
        REQUIRE(arr.Contains(300));
    }

    SECTION("Remove last element") {
        FixedVector<int, 10> arr;
        arr.Push(1);
        arr.Push(2);
        arr.Push(3);

        arr.RemoveUnorderedAt(2);  // Last index

        REQUIRE(arr.Size == 2);
        REQUIRE(arr.Contains(1));
        REQUIRE(arr.Contains(2));
        REQUIRE_FALSE(arr.Contains(3));
    }

    SECTION("Remove from single element array") {
        FixedVector<int, 10> arr;
        arr.Push(999);

        arr.RemoveUnorderedAt(0);

        REQUIRE(arr.Size == 0);
        REQUIRE(arr.IsEmpty());
    }


    SECTION("Verify unordered nature - element order after removal") {
        FixedVector<char, 10> arr;
        arr.Push('A');
        arr.Push('B');
        arr.Push('C');
        arr.Push('D');
        arr.Push('E');

        // Remove element at index 1 ('B')
        arr.RemoveUnorderedAt(1);

        REQUIRE(arr.Size == 4);

        // The last element 'E' should have moved to index 1
        REQUIRE(arr[0] == 'A');
        REQUIRE(arr[1] == 'E');  // Last element moved here
        REQUIRE(arr[2] == 'C');
        REQUIRE(arr[3] == 'D');

        REQUIRE_FALSE(arr.Contains('B'));
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

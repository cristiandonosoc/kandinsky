#include <catch2/catch_test_macros.hpp>

#include <kandinsky/memory.h>
#include <kandinsky/string.h>

using namespace kdk;
using namespace kdk::paths;

#define CREATE_ARENA()                          \
    Arena arena = AllocateArena(16 * KILOBYTE); \
    DEFER { FreeArena(&arena); };

// String ------------------------------------------------------------------------------------------

TEST_CASE("String") {
    SECTION("Empty") {
        String empty(nullptr, 1000);
        CHECK(!empty.Equals(nullptr));
        CHECK(!empty.Equals(""));

        empty = String("");
        CHECK(!empty.Equals(nullptr));
        CHECK(empty.Equals(""));
    }

    SECTION("Comparison") {
        String some("0123456789");
        CHECK(some.Equals("0123456789"));
        CHECK(!some.Equals("012345678"));
        CHECK(some.Equals(some));

        String other = some;
        CHECK(other.Equals(some));

        other = String(some.Str(), 5);
        CHECK(!other.Equals(some));
        CHECK(other.Equals("01234"));
    }
}

// Tests begin here
TEST_CASE("String construction and basic properties", "[string]") {
    SECTION("Default constructor creates empty string") {
        String s;
        INFO("Default constructed String should have _Str equal to kEmptyStrPtr");
        CHECK(s._Str == String::kEmptyStrPtr);
        INFO("Default constructed String should have Size 0");
        CHECK(s.Size == 0);
        INFO("Default constructed String should be empty");
        CHECK(s.IsEmpty());
        INFO("Default constructed String should be valid");
        CHECK(s.IsValid());
        INFO("Default constructed String's Str() should return empty string");
        CHECK(std::strcmp(s.Str(), "") == 0);
    }

    SECTION("Construction from C-string") {
        const char* test_str = "Hello, World!";
        String s(test_str);

        INFO("String constructed from C-string should store the pointer");
        CHECK(s._Str == test_str);
        INFO("Size should be set to the length of the string");
        CHECK(s.Size == std::strlen(test_str));
        INFO("IsEmpty should return false for non-empty string");
        CHECK_FALSE(s.IsEmpty());
        INFO("IsValid should return true for non-null string");
        CHECK(s.IsValid());
        INFO("Str() should return the original string");
        CHECK(s.Str() == test_str);
    }

    SECTION("Construction with explicit size") {
        const char* test_str = "Hello, World!";
        u64 test_size = 5;  // Only "Hello"
        String s(test_str, test_size);

        INFO("String constructed with explicit size should store the pointer");
        CHECK(s._Str == test_str);
        INFO("Size should match the provided value");
        CHECK(s.Size == test_size);
        INFO("IsEmpty should return false when size > 0");
        CHECK_FALSE(s.IsEmpty());
        INFO("IsValid should return true for non-null string");
        CHECK(s.IsValid());
    }

    SECTION("Construction with nullptr") {
        String s(nullptr, 0);

        INFO("String constructed with nullptr should have null _Str");
        CHECK(s._Str == nullptr);
        INFO("String constructed with nullptr should have Size 0");
        CHECK(s.Size == 0);
        INFO("String constructed with nullptr should be empty");
        CHECK(s.IsEmpty());
        INFO("String constructed with nullptr should not be valid");
        CHECK_FALSE(s.IsValid());
        INFO("String constructed with nullptr's Str() should return empty string");
        CHECK(std::strcmp(s.Str(), "") == 0);
    }
}

TEST_CASE("String's Str() method", "[string]") {
    SECTION("Str() should return the string when _Str is not null") {
        const char* test_str = "Test String";
        String s(test_str);

        INFO("Str() should return the original string pointer");
        CHECK(s.Str() == test_str);
    }

    SECTION("Str() should return kEmptyStrPtr when _Str is null") {
        String s(nullptr, 0);

        INFO("Str() should return kEmptyStrPtr for null _Str");
        CHECK(s.Str() == String::kEmptyStrPtr);
        INFO("Str() should never return nullptr");
        CHECK(s.Str() != nullptr);
    }
}

TEST_CASE("String's IsEmpty() and IsValid() methods", "[string]") {
    SECTION("IsEmpty() should return true for empty strings") {
        String s1;
        String s2("", 0);
        String s3(nullptr, 0);

        INFO("Default constructed string should be empty");
        CHECK(s1.IsEmpty());
        INFO("String with empty C-string should be empty");
        CHECK(s2.IsEmpty());
        INFO("String with nullptr should be empty");
        CHECK(s3.IsEmpty());
    }

    SECTION("IsEmpty() should return false for non-empty strings") {
        String s1("Hello");
        String s2("", 5);  // This is a bit strange but Size is non-zero

        INFO("String with content should not be empty");
        CHECK_FALSE(s1.IsEmpty());
        INFO("String with non-zero size should not be empty, even if _Str is empty");
        CHECK_FALSE(s2.IsEmpty());
    }

    SECTION("IsValid() should return true for non-null _Str") {
        String s1;
        String s2("Hello");
        String s3("", 0);

        INFO("Default constructed string should be valid");
        CHECK(s1.IsValid());
        INFO("String with content should be valid");
        CHECK(s2.IsValid());
        INFO("String with empty C-string should be valid");
        CHECK(s3.IsValid());
    }

    SECTION("IsValid() should return false for null _Str") {
        String s(nullptr, 0);

        INFO("String with nullptr should not be valid");
        CHECK_FALSE(s.IsValid());
    }
}

TEST_CASE("String's Equals(const char*) method", "[string]") {
    SECTION("Equals should compare content for equality") {
        const char* test_str = "Test String";
        String s(test_str);

        INFO("Equals should return true for identical C-strings");
        CHECK(s.Equals(test_str));
        INFO("Equals should return true for C-strings with same content");
        CHECK(s.Equals("Test String"));
        INFO("Equals should return false for C-strings with different content");
        CHECK_FALSE(s.Equals("Different String"));
        INFO("Equals should return false for nullptr");
        CHECK_FALSE(s.Equals(nullptr));
    }

    SECTION("Equals should handle empty strings") {
        String empty1;
        String empty2("", 0);

        INFO("Empty string should equal empty C-string");
        CHECK(empty1.Equals(""));
        CHECK(empty2.Equals(""));
        INFO("Empty string should not equal non-empty C-string");
        CHECK_FALSE(empty1.Equals("Not Empty"));
        CHECK_FALSE(empty2.Equals("Not Empty"));
    }

    SECTION("Equals should respect Size limit") {
        const char* test_str = "Test String Long";
        String s(test_str, 4);  // Only "Test"

        INFO("Equals should only compare up to Size characters");
        CHECK(s.Equals("Test"));
        CHECK_FALSE(s.Equals("Tes"));
        CHECK_FALSE(s.Equals("Testing"));
        CHECK_FALSE(s.Equals("Test String Long"));
    }

    SECTION("Equals should handle invalid strings") {
        String invalid(nullptr, 0);

        INFO("Invalid string should not equal any C-string");
        CHECK_FALSE(invalid.Equals(""));
        CHECK_FALSE(invalid.Equals("Test"));
        CHECK_FALSE(invalid.Equals(nullptr));
    }
}

TEST_CASE("String's Equals(const String&) method", "[string]") {
    SECTION("Equals should compare content for equality") {
        const char* test_str = "Test String";
        String s1(test_str);
        String s2("Test String");
        String s3("Different String");

        INFO("Equals should return true for strings with same content");
        CHECK(s1.Equals(s2));
        CHECK(s2.Equals(s1));
        INFO("Equals should return false for strings with different content");
        CHECK_FALSE(s1.Equals(s3));
        CHECK_FALSE(s3.Equals(s1));
    }

    SECTION("Equals should handle empty strings") {
        String empty1;
        String empty2("", 0);
        String nonEmpty("Not Empty");

        INFO("Empty strings should equal each other");
        CHECK(empty1.Equals(empty2));
        CHECK(empty2.Equals(empty1));
        INFO("Empty string should not equal non-empty string");
        CHECK_FALSE(empty1.Equals(nonEmpty));
        CHECK_FALSE(nonEmpty.Equals(empty1));
    }

    SECTION("Equals should respect Size limit") {
        const char* test_str = "Test String Long";
        String s1(test_str, 4);  // Only "Test"
        String s2("Test");
        String s3("Test String Long");

        INFO("Equals should only compare up to Size characters");
        CHECK(s1.Equals(s2));
        CHECK(s2.Equals(s1));
        CHECK_FALSE(s1.Equals(s3));
        CHECK_FALSE(s3.Equals(s1));
    }

    SECTION("Equals should optimize for same pointer") {
        const char* test_str = "Test String";
        String s1(test_str);
        String s2(test_str);

        INFO("Strings with same pointer should be equal without character comparison");
        CHECK(s1.Equals(s2));
    }

    SECTION("Equals should handle invalid strings") {
        String valid("Test");
        String invalid(nullptr, 0);
        String invalid2(nullptr, 0);

        INFO("Valid string should not equal invalid string");
        CHECK_FALSE(valid.Equals(invalid));

        INFO("Two invalid strings with same size should be equal");
        CHECK(invalid.Equals(invalid2));
    }
}

TEST_CASE("String edge cases and corner cases", "[string]") {
    SECTION("Strings with size but empty content") {
        String s("", 5);

        INFO("String should not be empty if Size > 0 regardless of content");
        CHECK_FALSE(s.IsEmpty());
        INFO("String should be valid if _Str is not nullptr");
        CHECK(s.IsValid());

        INFO("String with empty content but Size > 0 should not equal empty string");
        CHECK_FALSE(s.Equals(""));
        CHECK_FALSE(s.Equals(String()));
    }

    SECTION("String with null terminator in the middle") {
        const char str[] = {'H', 'e', 'l', 'l', 'o', '\0', 'W', 'o', 'r', 'l', 'd'};
        String s(str, sizeof(str));

        INFO("Size should include characters after null terminator");
        CHECK(s.Size == sizeof(str));

        char comparison[sizeof(str)];
        std::memcpy(comparison, str, sizeof(str));
        String s2(comparison, sizeof(comparison));

        INFO("Equals should compare all characters including after null terminator");
        CHECK(s.Equals(s2));

        // Change a character after the null terminator
        comparison[7] = 'X';  // Change 'o' to 'X'
        String s3(comparison, sizeof(comparison));

        INFO("Equals should detect differences after null terminator");
        CHECK_FALSE(s.Equals(s3));
    }

    SECTION("String equality with different pointers but same content") {
        char str1[] = "Test String";
        char str2[] = "Test String";

        String s1(str1);
        String s2(str2);

        INFO("Different pointers with same content should be equal");
        CHECK(s1._Str != s2._Str);  // Confirm different pointers
        CHECK(s1.Equals(s2));
        CHECK(s2.Equals(s1));
    }
}

TEST_CASE("String equality with different lengths", "[string]") {
    SECTION("Different lengths should not be equal") {
        String s1("Test");
        String s2("Testing");

        INFO("Strings with different lengths should not be equal");
        CHECK_FALSE(s1.Equals(s2));
        CHECK_FALSE(s2.Equals(s1));

        // Test with C-string comparison too
        CHECK_FALSE(s1.Equals("Testing"));
        CHECK_FALSE(s2.Equals("Test"));
    }

    SECTION("Prefix match should not imply equality") {
        String s1("Test");
        String s2("Testing");

        INFO("String should not equal its prefix or extension");
        CHECK_FALSE(s1.Equals(s2));
        CHECK_FALSE(s2.Equals(s1));
    }
}

TEST_CASE("Concat tests", "[string][concat]") {
    SECTION("Empty strings") {
        CREATE_ARENA();
        String a;
        String b;
        String result = Concat(&arena, a, b);
        CHECK(result.IsEmpty());
        CHECK(result.Size == 0);
    }

    SECTION("First string empty, second non-empty") {
        CREATE_ARENA();
        String a;
        String b("hello");
        String result = Concat(&arena, a, b);
        CHECK_FALSE(result.IsEmpty());
        CHECK(result.Size == 5);
        CHECK(std::strcmp(result.Str(), "hello") == 0);
    }

    SECTION("First string non-empty, second empty") {
        CREATE_ARENA();
        String a("world");
        String b;
        String result = Concat(&arena, a, b);
        CHECK_FALSE(result.IsEmpty());
        CHECK(result.Size == 5);
        CHECK(std::strcmp(result.Str(), "world") == 0);
    }

    SECTION("Both strings non-empty") {
        CREATE_ARENA();
        String a("hello");
        String b("world");
        String result = Concat(&arena, a, b);
        CHECK_FALSE(result.IsEmpty());
        CHECK(result.Size == 10);
        CHECK(std::strcmp(result.Str(), "helloworld") == 0);
    }

    SECTION("Concatenate with space") {
        CREATE_ARENA();
        String a("hello ");
        String b("world");
        String result = Concat(&arena, a, b);
        CHECK_FALSE(result.IsEmpty());
        CHECK(result.Size == 11);
        CHECK(std::strcmp(result.Str(), "hello world") == 0);
    }

    SECTION("Single character strings") {
        CREATE_ARENA();
        String a("a");
        String b("b");
        String result = Concat(&arena, a, b);
        CHECK_FALSE(result.IsEmpty());
        CHECK(result.Size == 2);
        CHECK(std::strcmp(result.Str(), "ab") == 0);
    }

    SECTION("Concatenate longer strings") {
        CREATE_ARENA();
        String a("The quick brown fox ");
        String b("jumps over the lazy dog");
        String result = Concat(&arena, a, b);
        CHECK_FALSE(result.IsEmpty());
        CHECK(result.Size == 43);
        CHECK(std::strcmp(result.Str(), "The quick brown fox jumps over the lazy dog") == 0);
    }

    SECTION("Concatenate strings with null bytes") {
        CREATE_ARENA();
        String a("hello\0world", 11);
        String b("test", 4);
        String result = Concat(&arena, a, b);
        CHECK_FALSE(result.IsEmpty());
        CHECK(result.Size == 15);
        // Note: strcmp won't work here due to null byte, so check manually
        CHECK(std::memcmp(result.Str(), "hello\0worldtest", 15) == 0);
    }

    SECTION("Result string properties") {
        CREATE_ARENA();
        String a("foo");
        String b("bar");
        String result = Concat(&arena, a, b);

        CHECK(result.IsValid());
        CHECK_FALSE(result.IsEmpty());
        CHECK(result.Str() != nullptr);
        CHECK(result.Str() != a.Str());  // Should be a new allocation
        CHECK(result.Str() != b.Str());  // Should be a new allocation
    }

    SECTION("Equality checks after concatenation") {
        CREATE_ARENA();
        String a("test");
        String b("ing");
        String result = Concat(&arena, a, b);
        String expected("testing");

        CHECK(result.Equals(expected));
        CHECK(result == expected);
        CHECK(result.Equals("testing"));
    }
}

// Paths -------------------------------------------------------------------------------------------

TEST_CASE("GetDirname tests", "[path]") {
    SECTION("Empty path") {
        CREATE_ARENA();
        String path;
        String result = GetDirname(&arena, path);
        CHECK(result.IsEmpty());
    }

    SECTION("Path with no directory component") {
        CREATE_ARENA();
        String path("filename.txt");
        String result = GetDirname(&arena, path);
        CHECK(result.IsEmpty());
    }

    SECTION("Path with directory component") {
        CREATE_ARENA();
        String path("/path/to/file.txt");
        String result = GetDirname(&arena, path);
        const char* want = "/path/to/";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Path with trailing slash") {
        CREATE_ARENA();
        String path("/path/to/");
        String result = GetDirname(&arena, path);
        const char* want = "/path/";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Root directory") {
        CREATE_ARENA();
        String path("/file.txt");
        String result = GetDirname(&arena, path);
        const char* want = "/";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Windows-style paths") {
        CREATE_ARENA();
        String path("C:\\path\\to\\file.txt");
        String result = GetDirname(&arena, path);
        const char* want = "C:\\path\\to\\";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Windows-style paths: Root") {
        CREATE_ARENA();
        String path("C:\\");
        String result = GetDirname(&arena, path);
        const char* want = "";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Path with multiple slashes") {
        CREATE_ARENA();
        String path("/path//to/file.txt");
        String result = GetDirname(&arena, path);
        const char* want = "/path//to/";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }
}

TEST_CASE("GetBasename tests", "[path]") {
    SECTION("Empty path") {
        CREATE_ARENA();
        String path;
        String result = GetBasename(&arena, path);
        CHECK(result.IsEmpty());
    }

    SECTION("Path with no directory component") {
        CREATE_ARENA();
        String path("filename.txt");
        String result = GetBasename(&arena, path);
        const char* want = "filename.txt";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Path with directory component") {
        CREATE_ARENA();
        String path("/path/to/file.txt");
        String result = GetBasename(&arena, path);
        const char* want = "file.txt";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Path with trailing slash") {
        CREATE_ARENA();
        String path("/path/to/");
        String result = GetBasename(&arena, path);
        const char* want = "to";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Root directory") {
        CREATE_ARENA();
        String path("/");
        String result = GetBasename(&arena, path);
        const char* want = "";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Windows-style paths") {
        CREATE_ARENA();
        String path("C:\\path\\to\\file.txt");
        String result = GetBasename(&arena, path);
        const char* want = "file.txt";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Windows-style paths: Dir") {
        CREATE_ARENA();
        String path("C:\\path\\to\\");
        String result = GetBasename(&arena, path);
        const char* want = "to";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Path with multiple extensions") {
        CREATE_ARENA();
        String path("/path/to/file.tar.gz");
        String result = GetBasename(&arena, path);
        const char* want = "file.tar.gz";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }
}

TEST_CASE("GetExtension tests", "[path]") {
    SECTION("Empty path") {
        CREATE_ARENA();
        String path;
        String result = GetExtension(&arena, path);
        CHECK(result.IsEmpty());
    }

    SECTION("Path with no extension") {
        CREATE_ARENA();
        String path("filename");
        String result = GetExtension(&arena, path);
        CHECK(result.IsEmpty());
    }

    SECTION("Path with extension") {
        CREATE_ARENA();
        String path("filename.txt");
        String result = GetExtension(&arena, path);
        const char* want = ".txt";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Path with multiple extensions") {
        CREATE_ARENA();
        String path("filename.tar.gz");
        String result = GetExtension(&arena, path);
        const char* want = ".gz";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Path with directory and extension") {
        CREATE_ARENA();
        String path("/path/to/file.txt");
        String result = GetExtension(&arena, path);
        const char* want = ".txt";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Path with dot in directory name") {
        CREATE_ARENA();
        String path("/path.with.dots/filename");
        String result = GetExtension(&arena, path);
        CHECK(result.IsEmpty());
    }

    SECTION("Path with hidden file (dot at start)") {
        CREATE_ARENA();
        String path(".hidden");
        String result = GetExtension(&arena, path);
        const char* want = ".hidden";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Path with hidden file with extension") {
        CREATE_ARENA();
        String path(".hidden.txt");
        String result = GetExtension(&arena, path);
        const char* want = ".txt";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }
}

TEST_CASE("RemoveExtension tests", "[path]") {
    SECTION("Empty path") {
        CREATE_ARENA();
        String path;
        String result = RemoveExtension(&arena, path);
        CHECK(result.IsEmpty());
    }

    SECTION("Path with no extension") {
        CREATE_ARENA();
        String path("filename");
        String result = RemoveExtension(&arena, path);
        const char* want = "filename";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Path with extension") {
        CREATE_ARENA();
        String path("filename.txt");
        String result = RemoveExtension(&arena, path);
        const char* want = "filename";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Path with multiple extensions") {
        CREATE_ARENA();
        String path("filename.tar.gz");
        String result = RemoveExtension(&arena, path);
        const char* want = "filename.tar";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Path with directory and extension") {
        CREATE_ARENA();
        String path("/path/to/file.txt");
        String result = RemoveExtension(&arena, path);
        const char* want = "/path/to/file";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Path with dot in directory name") {
        CREATE_ARENA();
        String path("/path.with.dots/filename.txt");
        String result = RemoveExtension(&arena, path);
        const char* want = "/path.with.dots/filename";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Path with hidden file (dot at start)") {
        CREATE_ARENA();
        String path(".hidden");
        String result = RemoveExtension(&arena, path);
        const char* want = ".hidden";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Path with hidden file with extension") {
        CREATE_ARENA();
        String path(".hidden.txt");
        String result = RemoveExtension(&arena, path);
        const char* want = ".hidden";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }
}

TEST_CASE("PathJoin basic functionality", "[pathjoin]") {
    SECTION("Joining two non-empty paths") {
        CREATE_ARENA();
        String a("dir");
        String b("file.txt");
        String result = PathJoin(&arena, a, b);

        INFO("Should join paths with a separator");
        const char* want = "dir\\file.txt";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Joining path with trailing slash") {
        CREATE_ARENA();
        String a("dir/");
        String b("file.txt");
        String result = PathJoin(&arena, a, b);

        INFO("Should preserve trailing slash and not add another");
        const char* want = "dir\\file.txt";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Joining with absolute path as second parameter") {
        CREATE_ARENA();
        String a("dir");
        String b("/file.txt");
        String result = PathJoin(&arena, a, b);

        INFO("Should handle absolute path as second parameter");
        const char* want = "dir\\file.txt";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }
}

// TODO(cdc): For the PathJoin tests we use windows \\ as separators, but we should do something
//            Platform independent. These tests will fail on Linux, even though the logic is most
//            likely correct.

TEST_CASE("PathJoin with empty inputs", "[pathjoin]") {
    SECTION("First path is empty") {
        CREATE_ARENA();
        String a;
        String b("file.txt");
        String result = PathJoin(&arena, a, b);

        INFO("Should return second path when first is empty");
        CHECK(result._Str == b._Str);  // Should return the exact same string object

        const char* want = "file.txt";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Second path is empty") {
        CREATE_ARENA();
        String a("dir");
        String b;
        String result = PathJoin(&arena, a, b);

        INFO("Should return first path when second is empty");
        CHECK(result._Str == a._Str);  // Should return the exact same string object

        const char* want = "dir";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Both paths are empty") {
        CREATE_ARENA();
        String a;
        String b;
        String result = PathJoin(&arena, a, b);

        INFO("Should return empty string when both inputs are empty");
        CHECK(result.IsEmpty());
    }
}

TEST_CASE("PathJoin with nested paths", "[pathjoin]") {
    SECTION("Joining multiple directory levels") {
        CREATE_ARENA();
        String a("dir1/dir2");
        String b("file.txt");
        String result = PathJoin(&arena, a, b);

        INFO("Should correctly join nested directories");
        const char* want = "dir1\\dir2\\file.txt";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Joining path with filename and extension") {
        CREATE_ARENA();
        String a("dir");
        String b("subdir/file.txt");
        String result = PathJoin(&arena, a, b);

        INFO("Should join path with subdirectory and filename");
        const char* want = "dir\\subdir\\file.txt";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }
}

TEST_CASE("PathJoin with different slashes", "[pathjoin]") {
    SECTION("Joining paths with forward slashes") {
        CREATE_ARENA();
        String a("dir/");
        String b("file.txt");
        String result = PathJoin(&arena, a, b);

        INFO("Should handle forward slashes correctly");
        const char* want = "dir\\file.txt";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Joining paths with backslashes") {
        CREATE_ARENA();
        String a("dir\\");
        String b("file.txt");
        String result = PathJoin(&arena, a, b);

        INFO("Should handle backslashes correctly");
        const char* want = "dir\\file.txt";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Joining paths with mixed slashes") {
        CREATE_ARENA();
        String a("dir1/dir2\\");
        String b("file.txt");
        String result = PathJoin(&arena, a, b);

        INFO("Should handle mixed slashes according to implementation");
        const char* want = "dir1\\dir2\\file.txt";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }
}

TEST_CASE("PathJoin special cases", "[pathjoin]") {
    SECTION("Joining root directory") {
        CREATE_ARENA();
        String a("/");
        String b("file.txt");
        String result = PathJoin(&arena, a, b);

        INFO("Should correctly join root directory");
        const char* want = "\\file.txt";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Joining with current directory") {
        CREATE_ARENA();
        String a(".");
        String b("file.txt");
        String result = PathJoin(&arena, a, b);

        INFO("Should handle current directory reference");
        const char* want = "file.txt";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }

    SECTION("Joining with parent directory") {
        CREATE_ARENA();
        String a("..");
        String b("file.txt");
        String result = PathJoin(&arena, a, b);

        INFO("Should handle parent directory reference");
        const char* want = "..\\file.txt";
        INFO("Want: " << want << ", Got: " << result.Str());
        CHECK(result.Equals(want));
    }
}

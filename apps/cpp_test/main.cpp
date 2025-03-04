#include <kandinsky/memory.h>
#include <kandinsky/string.h>

#include <iterator>

struct Foo {
    int Bar = 0;
};

struct Test {
    Foo* Foos[4] = {};
};

int main() {
    using namespace kdk;
    Foo foo1;
    Foo foo2;

    {
        Test test;
        for (size_t i = 0; i < std::size(test.Foos); i++) {
            printf("%llu: %p\n", i, (void*)test.Foos[i]);
        }
    }

    {
        Test test = {
            .Foos =
                {
                       &foo1,
                       &foo2,
                       },
        };
        for (size_t i = 0; i < std::size(test.Foos); i++) {
            printf("%llu: %p\n", i, (void*)test.Foos[i]);
        }
    }

    Arena arena = AllocateArena(1 * MEGABYTE);

    const char* path = "assets/models";
    if (auto result = paths::ListDir(&arena, String(path)); IsValid(result)) {
        for (u32 i = 0; i < result.Count; i++) {
            printf("- %s\n", result.Entries[i].Str());
        }
    }
}

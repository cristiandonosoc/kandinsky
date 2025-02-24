#include <iterator>

struct Foo {
    int Bar = 0;
};

struct Test {
    Foo* Foos[4] = {};
};

int main() {
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
}

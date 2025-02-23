#include <iterator>

struct Foo {
    int Bar = 0;
};

struct Test {
    Foo* Foos[4] = {};
};


int main() {
    {
        Test test;
        for (size_t i = 0; i < std::size(test.Foos); i++) {
            printf("%llu: %p\n", i, (void*)test.Foos[i]);
        }
    }


    {
        Test test = {};
        for (size_t i = 0; i < std::size(test.Foos); i++) {
            printf("%llu: %p\n", i, (void*)test.Foos[i]);
        }
    }
}

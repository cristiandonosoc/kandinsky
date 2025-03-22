#include <cstdio>

#ifdef KDK_ATTRIBUTE_GENERATION
#define KDK_ATTR(...) __attribute__((annotate("KDK", __VA_ARGS__)))
#else
#define KDK_ATTR(...)
#endif

struct KDK_ATTR("imgui", "foo", "bar") Camera {
    int a;
};
int main() { printf("Hello, World!\n"); }

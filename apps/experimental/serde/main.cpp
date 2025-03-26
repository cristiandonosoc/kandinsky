#include <experimental/serde/serde.h>

#include <iostream>

namespace kdk {

struct Bar {
    String Name;

    Transform Transform = {};

    DynArray<String> Addresses = {};
    DynArray<Vec3> Positions = {};
};

void Serialize(SerdeArchive* ar, Bar& bar) {
    SERDE(ar, bar, Name);
    SERDE(ar, bar, Transform);
    SERDE(ar, bar, Addresses);
    SERDE(ar, bar, Positions);
}

struct Foo {
    String Name = {};
    int Age = 0;
    float Height = 0.0f;

    DynArray<int> Ints = {};
    DynArray<Bar> Bars = {};
};

void Serialize(SerdeArchive* ar, Foo& foo) {
    YAML::Node node;

    SERDE(ar, foo, Name);
    SERDE(ar, foo, Age);
    SERDE(ar, foo, Height);
    SERDE(ar, foo, Ints);
    SERDE(ar, foo, Bars);
}

}  // namespace kdk

int main() {
    using namespace kdk;

    // Create an arena for our allocations
    Arena arena = AllocateArena(1 * MEGABYTE);
    DEFER { FreeArena(&arena); };

    // Serialize
    SerdeArchive sa = NewSerdeArchive(&arena, ESerdeBackend::YAML, ESerdeMode::Serialize);

    // Create example data
    Foo foo;
    foo.Name = String("Example Foo");
    foo.Age = 25;
    foo.Height = 1.75f;
    foo.Ints = NewDynArray<int>(&arena);
    foo.Ints.Push(&arena, 1);
    foo.Ints.Push(&arena, 2);
    foo.Ints.Push(&arena, 3);

    // Add some bars
    Bar bar1;
    bar1.Name = String("Bar One");
    bar1.Transform.Position = {1.0f, 2.0f, 3.0f};
    bar1.Transform.Rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    bar1.Transform.Scale = 1.0f;
    bar1.Addresses = NewDynArray<String>(&arena);
    bar1.Addresses.Push(&arena, String("Address 1"));
    bar1.Addresses.Push(&arena, String(R"(This is a long string
that spans multiple lines.
It preserves newlines and special characters.)"));
    bar1.Positions.Push(&arena, Vec3(1, 0, 0));
    bar1.Positions.Push(&arena, Vec3(0, 1, 0));
    bar1.Positions.Push(&arena, Vec3(0, 0, 1));

    Bar bar2 = bar1;
    bar2.Name = String("Bar Two");

    foo.Bars.Push(&arena, bar1);
    foo.Bars.Push(&arena, bar2);

    Serde(&sa, "Foo", foo);

    std::cout << sa.BaseNode << std::endl;

    // // Save to file
    // SaveToFile(serde, "example.yaml");
    return 0;
}

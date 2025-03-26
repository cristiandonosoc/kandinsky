#include <cstdio>

#include <kandinsky/container.h>
#include <kandinsky/math.h>
#include <kandinsky/memory.h>
#include <kandinsky/string.h>
#include <yaml-cpp/yaml.h>

#include <fstream>
#include <iostream>

namespace kdk {

struct Bar {
    String Name;

    Transform Transform = {};

    DynArray<int> Ints = {};
    DynArray<String> Addresses = {};
};

struct Foo {
    String Name = {};
    int Age = 0;
    float Height = 0.0f;

    DynArray<Bar> Bars = {};
};

struct SerdeArchive {
    Arena* Arena = nullptr;
    YAML::Node BaseNode;
    YAML::Node* CurrentNode = nullptr;
};

SerdeArchive NewSerdeArchive(Arena* arena) {
    SerdeArchive serde;
    serde.Arena = arena;
    serde.BaseNode = YAML::Node();
    serde.CurrentNode = &serde.BaseNode;
    return serde;
}

template <typename T>
void Serde(SerdeArchive* ar, const char* name, T& t) {
    auto* prev = ar->CurrentNode;

    YAML::Node node;
    ar->CurrentNode = &node;
    Serialize(ar, t);

    (*prev)[name] = node;
    ar->CurrentNode = prev;
}

template <>
void Serde<String>(SerdeArchive* ar, const char* name, String& value) {
    (*ar->CurrentNode)[name] = value.Str();
}

template <>
void Serde<int>(SerdeArchive* ar, const char* name, int& value) {
    (*ar->CurrentNode)[name] = value;
}

template <>
void Serde<float>(SerdeArchive* ar, const char* name, float& value) {
    (*ar->CurrentNode)[name] = value;
}

template <>
void Serde<Vec3>(SerdeArchive* ar, const char* name, Vec3& value) {
    YAML::Node node;
    node["x"] = value.x;
    node["y"] = value.y;
    node["z"] = value.z;
    (*ar->CurrentNode)[name] = node;
}

template <>
void Serde<Quat>(SerdeArchive* ar, const char* name, Quat& value) {
    YAML::Node node;
    node["x"] = value.x;
    node["y"] = value.y;
    node["z"] = value.z;
    node["w"] = value.w;
    (*ar->CurrentNode)[name] = node;
}

template <>
void Serde<Transform>(SerdeArchive* ar, const char* name, Transform& value) {
    auto* prev = ar->CurrentNode;

    YAML::Node node;
    ar->CurrentNode = &node;
    Serde(ar, "Position", value.Position);
    Serde(ar, "Rotation", value.Rotation);
    Serde(ar, "Scale", value.Scale);

    (*prev)[name] = node;
    ar->CurrentNode = prev;
}

void Serialize(SerdeArchive* ar, Bar& bar) {
    YAML::Node node;

    Serde(ar, "Name", bar.Name);
    Serde(ar, "Transform", bar.Transform);

    // // Serialize integer array
    // YAML::Node ints;
    // for (u32 i = 0; i < bar.Ints.Size; ++i) {
    //     ints.push_back(bar.Ints[i]);
    // }
    // node["ints"] = ints;

    // // Serialize string array
    // YAML::Node addresses;
    // for (u32 i = 0; i < bar.Addresses.Size; ++i) {
    //     addresses.push_back(bar.Addresses[i].Str());
    // }
    // node["addresses"] = addresses;
}

void Serialize(SerdeArchive* ar, Foo& foo) {
    YAML::Node node;

    Serde(ar, "Name", foo.Name);
    Serde(ar, "Age", foo.Age);
    Serde(ar, "Height", foo.Height);

    // // Serialize Bar array
    // YAML::Node bars;
    // for (u32 i = 0; i < foo.Bars.Size; ++i) {
    //     Serde barSerde;
    //     Serialize(&barSerde, arena, foo.Bars[i]);
    //     bars.push_back(barSerde.node);
    // }
    // node["bars"] = bars;

    // serde->node = node;
}

}  // namespace kdk

int main() {
    using namespace kdk;

    // Create an arena for our allocations
    Arena arena = AllocateArena(1 * MEGABYTE);
    DEFER { FreeArena(&arena); };

    // Serialize
    SerdeArchive ar = NewSerdeArchive(&arena);

    // Create example data
    Foo foo;
    foo.Name = String("Example Foo");
    foo.Age = 25;
    foo.Height = 1.75f;

    // Add some bars
    Bar bar1;
    bar1.Name = String("Bar One");
    bar1.Transform.Position = {1.0f, 2.0f, 3.0f};
    bar1.Transform.Rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    bar1.Transform.Scale = 1.0f;
    bar1.Ints = NewDynArray<int>(&arena);
    bar1.Ints.Push(&arena, 1);
    bar1.Ints.Push(&arena, 2);
    bar1.Ints.Push(&arena, 3);
    bar1.Addresses = NewDynArray<String>(&arena);
    bar1.Addresses.Push(&arena, String("Address 1"));
    bar1.Addresses.Push(&arena, String("Address 2"));

    foo.Bars = NewDynArray<Bar>(&arena);
    foo.Bars.Push(&arena, bar1);

    Serde(&ar, "Foo", foo);

    std::cout << ar.BaseNode << std::endl;

    // // Save to file
    // SaveToFile(serde, "example.yaml");

    printf("Serialization complete! Check example.yaml\n");
    return 0;
}

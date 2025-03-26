#include <kandinsky/container.h>
#include <kandinsky/math.h>
#include <kandinsky/memory.h>
#include <kandinsky/string.h>
#include <yaml-cpp/yaml.h>

#include <iostream>

namespace kdk {

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

    (*prev)[name] = std::move(node);
    ar->CurrentNode = prev;
}

#define SERDE(ar, owner, member) Serde(ar, #member, owner.member)

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

void SerdeInline(YAML::Node& node, Vec3& value) {
    node["x"] = value.x;
    node["y"] = value.y;
    node["z"] = value.z;
}

template <>
void Serde<Vec3>(SerdeArchive* ar, const char* name, Vec3& value) {
    YAML::Node node;
    SerdeInline(node, value);
    (*ar->CurrentNode)[name] = std::move(node);
}

void SerdeInline(YAML::Node& node, Quat& value) {
    node["x"] = value.x;
    node["y"] = value.y;
    node["z"] = value.z;
    node["w"] = value.w;
}

template <>
void Serde<Quat>(SerdeArchive* ar, const char* name, Quat& value) {
    YAML::Node node;
    SerdeInline(node, value);
    (*ar->CurrentNode)[name] = std::move(node);
}

template <>
void Serde<Transform>(SerdeArchive* ar, const char* name, Transform& value) {
    auto* prev = ar->CurrentNode;

    YAML::Node node;
    ar->CurrentNode = &node;
    SERDE(ar, value, Position);
    SERDE(ar, value, Rotation);
    SERDE(ar, value, Scale);

    (*prev)[name] = std::move(node);
    ar->CurrentNode = prev;
}

template <typename T>
concept HasInlineSerde = requires(T value) {
    { SerdeInline(std::declval<YAML::Node&>(), value) };
};

template <typename T>
void Serde(SerdeArchive* ar, const char* name, DynArray<T>& values) {
    auto* prev = ar->CurrentNode;
    YAML::Node array_node = YAML::Node(YAML::NodeType::Sequence);

    for (u32 i = 0; i < values.Size; i++) {
        auto& value = values[i];
        if constexpr (std::is_arithmetic_v<T>) {
            array_node.push_back(value);
        } else if constexpr (std::is_same_v<T, String>) {
            array_node.push_back(value.Str());
        } else if constexpr (HasInlineSerde<T>) {
            YAML::Node node;
            SerdeInline(node, value);
            array_node.push_back(std::move(node));
        } else {
            YAML::Node node;
            ar->CurrentNode = &node;
            Serialize(ar, value);
            array_node.push_back(std::move(node));
        }
    }

    (*prev)[name] = std::move(array_node);
    ar->CurrentNode = prev;
}

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
    SerdeArchive ar = NewSerdeArchive(&arena);

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

    Serde(&ar, "Foo", foo);

    std::cout << ar.BaseNode << std::endl;

    // // Save to file
    // SaveToFile(serde, "example.yaml");
    return 0;
}

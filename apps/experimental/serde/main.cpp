#include <kandinsky/core/string.h>
#include <kandinsky/serde.h>

#include <cstdlib>
#include <fstream>
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

std::string ToString(const Bar& bar) {
    std::stringstream ss;
    ss << "Bar {\n";
    ss << "  Name: " << bar.Name.Str() << "\n";
    ss << "  Transform: {\n";

    ss << "    Position: [" << bar.Transform.Position.x << ", " << bar.Transform.Position.y << ", "
       << bar.Transform.Position.z << "]\n";

    ss << "    Rotation: [" << bar.Transform.Rotation.x << ", " << bar.Transform.Rotation.y << ", "
       << bar.Transform.Rotation.z << ", " << bar.Transform.Rotation.w << "]\n";

    ss << "    Scale: [" << bar.Transform.Scale.x << ", " << bar.Transform.Scale.y << ", "
       << bar.Transform.Scale.z << "]\n";

    ss << "  }\n";

    ss << "  Addresses: [\n";
    for (u32 i = 0; i < bar.Addresses.Size; ++i) {
        ss << "    \"" << bar.Addresses[i].Str() << "\"\n";
    }
    ss << "  ]\n";

    ss << "  Positions: [\n";
    for (u32 i = 0; i < bar.Positions.Size; ++i) {
        ss << "    [" << bar.Positions[i].x << ", " << bar.Positions[i].y << ", "
           << bar.Positions[i].z << "]\n";
    }
    ss << "  ]\n";
    ss << "}";
    return ss.str();
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

std::string ToString(const Foo& foo) {
    std::stringstream ss;
    ss << "Foo {\n";
    ss << "  Name: \"" << foo.Name.Str() << "\"\n";
    ss << "  Age: " << foo.Age << "\n";
    ss << "  Height: " << foo.Height << "\n";

    ss << "  Ints: [";
    for (u32 i = 0; i < foo.Ints.Size; ++i) {
        if (i > 0) {
            ss << ", ";
        }
        ss << foo.Ints[i];
    }
    ss << "]\n";

    ss << "  Bars: [\n";
    for (u32 i = 0; i < foo.Bars.Size; ++i) {
        ss << "    " << ToString(foo.Bars[i]) << "\n";
    }
    ss << "  ]\n";
    ss << "}";
    return ss.str();
}

bool SaveToFile(const SerdeArchive& ar, String filename) {
    std::ofstream fout(filename.Str());
    if (!fout.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename.Str() << std::endl;
        return false;
    }
    fout << ar.BaseNode;
    fout.close();
    return true;
}

bool LoadFromFile(SerdeArchive* ar, String filename) {
    std::ifstream fin(filename.Str());
    if (!fin.is_open()) {
        std::cerr << "Failed to open file for reading: " << filename.Str() << std::endl;
        return false;
    }

    // Load the YAML content
    std::stringstream buffer;
    buffer << fin.rdbuf();
    fin.close();

    // Parse YAML
    ar->BaseNode = YAML::Load(buffer.str());

    // Verify the root node is valid
    if (!ar->BaseNode.IsDefined()) {
        std::cerr << "Failed to parse YAML from file: " << filename.Str() << std::endl;
        return false;
    }

    return true;
}

}  // namespace kdk

int main() {
    using namespace kdk;

    // Create an arena for our allocations
    Arena arena = AllocateArena(1 * MEGABYTE);
    DEFER { FreeArena(&arena); };

    String base_dir = paths::GetBaseDir(&arena);

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
    bar1.Transform.Scale = Vec3(1.0f);
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
    std::string original_foo = ToString(foo);

    std::cout << "Original object:" << std::endl;
    std::cout << original_foo << std::endl;

    // std::cout << "\nSerialized YAML:" << std::endl;
    // std::cout << sa.BaseNode << std::endl;

    // Save to file
    String filename = paths::PathJoin(&arena, base_dir, String("temp"), String("example.yaml"));
    if (SaveToFile(sa, filename)) {
        std::cout << "Wrote to " << filename.Str() << std::endl;
    } else {
        return -1;
    }

    // __debugbreak();
    sa.Mode = ESerdeMode::Deserialize;
    if (LoadFromFile(&sa, filename)) {
        Foo loaded_foo = {};
        Serde(&sa, "Foo", loaded_foo);
        std::string parsed_foo = ToString(loaded_foo);
        std::cout << "PARSED FOO: " << parsed_foo << std::endl;
        if (original_foo == parsed_foo) {
            std::cout << "Ser/De works!" << std::endl;
        }
    } else {
        return -1;
    }

    return 0;
}

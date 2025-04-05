#include <catch2/catch_test_macros.hpp>

#include <kandinsky/serde.h>

using namespace kdk;

namespace kdk {
namespace serde_test_private {

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

}  // namespace serde_test_private
}  // namespace kdk

TEST_CASE("Serde", "[serde]") {
    SECTION("serialization deserialization") {
        using namespace kdk::serde_test_private;
        Arena arena = AllocateArena(16 * MEGABYTE);
        DEFER { FreeArena(&arena); };

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
        bar1.Positions = NewDynArray<Vec3>(&arena);
        bar1.Positions.Push(&arena, Vec3(1, 0, 0));
        bar1.Positions.Push(&arena, Vec3(0, 1, 0));
        bar1.Positions.Push(&arena, Vec3(0, 0, 1));

        Bar bar2 = bar1;
        bar2.Name = String("Bar Two");

        foo.Bars = NewDynArray<Bar>(&arena);
        foo.Bars.Push(&arena, bar1);
        foo.Bars.Push(&arena, bar2);

        // Serialize to YAML
        SerdeArchive sa = NewSerdeArchive(&arena, ESerdeBackend::YAML, ESerdeMode::Serialize);
        Serde(&sa, "Foo", foo);

        // Convert to string for debugging
        std::string yaml_str = YAML::Dump(sa.BaseNode);
        INFO("Serialized YAML:\n" << yaml_str);

        // Deserialize from YAML
        sa.Mode = ESerdeMode::Deserialize;
        sa.BaseNode = YAML::Load(yaml_str);

        Foo deserialized_foo = {};
        Serde(&sa, "Foo", deserialized_foo);

        // Verify the deserialized data matches the original
        REQUIRE(deserialized_foo.Name == foo.Name);
        REQUIRE(deserialized_foo.Age == foo.Age);
        REQUIRE(deserialized_foo.Height == foo.Height);

        // Check Ints array
        REQUIRE(deserialized_foo.Ints.Size == foo.Ints.Size);
        for (u32 i = 0; i < foo.Ints.Size; ++i) {
            REQUIRE(deserialized_foo.Ints[i] == foo.Ints[i]);
        }

        // Check Bars array
        REQUIRE(deserialized_foo.Bars.Size == foo.Bars.Size);
        for (u32 i = 0; i < foo.Bars.Size; ++i) {
            const Bar& orig_bar = foo.Bars[i];
            const Bar& deser_bar = deserialized_foo.Bars[i];

            REQUIRE(deser_bar.Name == orig_bar.Name);

            // Check Transform
            REQUIRE(deser_bar.Transform.Position == orig_bar.Transform.Position);
            REQUIRE(deser_bar.Transform.Rotation == orig_bar.Transform.Rotation);
            REQUIRE(deser_bar.Transform.Scale == orig_bar.Transform.Scale);

            // Check Addresses
            REQUIRE(deser_bar.Addresses.Size == orig_bar.Addresses.Size);
            for (u32 j = 0; j < orig_bar.Addresses.Size; ++j) {
                REQUIRE(deser_bar.Addresses[j] == orig_bar.Addresses[j]);
            }

            // Check Positions
            REQUIRE(deser_bar.Positions.Size == orig_bar.Positions.Size);
            for (u32 j = 0; j < orig_bar.Positions.Size; ++j) {
                REQUIRE(deser_bar.Positions[j] == orig_bar.Positions[j]);
            }
        }
    }
}

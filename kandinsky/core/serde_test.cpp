#include <catch2/catch_test_macros.hpp>

#include <kandinsky/core/serde.h>

using namespace kdk;

namespace kdk {
namespace serde_test_private {

struct Bar {
    String Name;

    Transform Transform = {};

    DynArray<String> Addresses = {};
    FixedVector<FixedString<64>, 32> FixedStrings = {};
    DynArray<Vec3> Positions = {};
};

void Serialize(SerdeArchive* ar, Bar* bar) {
    SERDE(ar, bar, Name);
    SERDE(ar, bar, Transform);
    SERDE(ar, bar, Addresses);
    SERDE(ar, bar, FixedStrings);
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
    for (i32 i = 0; i < bar.Addresses.Size; ++i) {
        ss << "    \"" << bar.Addresses[i].Str() << "\"\n";
    }
    ss << "  ]\n";

    ss << "  FixedStrings: [\n";
    for (i32 i = 0; i < bar.Addresses.Size; ++i) {
        ss << "    \"" << bar.Addresses[i].Str() << "\"\n";
    }
    ss << "  ]\n";

    ss << "  Positions: [\n";
    for (i32 i = 0; i < bar.Positions.Size; ++i) {
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

void Serialize(SerdeArchive* ar, Foo* foo) {
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
    for (i32 i = 0; i < foo.Ints.Size; ++i) {
        if (i > 0) {
            ss << ", ";
        }
        ss << foo.Ints[i];
    }
    ss << "]\n";

    ss << "  Bars: [\n";
    for (i32 i = 0; i < foo.Bars.Size; ++i) {
        ss << "    " << ToString(foo.Bars[i]) << "\n";
    }
    ss << "  ]\n";
    ss << "}";
    return ss.str();
}

struct Test {
    u8 u8Value = 0;
    u16 u16Value = 0;
    u32 u32Value = 0;
    u64 u64Value = 0;
    i8 i8Value = 0;
    i16 i16Value = 0;
    i32 i32Value = 0;
    i64 i64Value = 0;
    f32 f32Value = 0;
    f64 f64Value = 0;
};

void Serialize(SerdeArchive* sa, Test* test) {
    SERDE(sa, test, u8Value);
    SERDE(sa, test, u16Value);
    SERDE(sa, test, u32Value);
    SERDE(sa, test, u64Value);
    SERDE(sa, test, i8Value);
    SERDE(sa, test, i16Value);
    SERDE(sa, test, i32Value);
    SERDE(sa, test, i64Value);
    SERDE(sa, test, f32Value);
    SERDE(sa, test, f64Value);
}

std::string ToString(const Test& test) {
    std::stringstream ss;

    ss << "Test:\n";

    ss << "  u8Value: " << test.u8Value << "\n";
    ss << "  u16Value: " << test.u16Value << "\n";
    ss << "  u32Value: " << test.u32Value << "\n";
    ss << "  u64Value: " << test.u64Value << "\n";
    ss << "  i8Value: " << test.i8Value << "\n";
    ss << "  i16Value: " << test.i16Value << "\n";
    ss << "  i32Value: " << test.i32Value << "\n";
    ss << "  i64Value: " << test.i64Value << "\n";
    ss << "  f32Value: " << test.f32Value << "\n";
    ss << "  f64Value: " << test.f64Value << "\n";

    ss << "}\n";
    return ss.str();
}

}  // namespace serde_test_private
}  // namespace kdk

TEST_CASE("Simple immediates", "[serde]") {
    using namespace kdk::serde_test_private;

    Arena arena = AllocateArena("TestArena"sv, 16 * MEGABYTE);
    DEFER { FreeArena(&arena); };

    Test test;
    test.u8Value = std::numeric_limits<u8>::max();
    test.u16Value = std::numeric_limits<u16>::max();
    test.u32Value = std::numeric_limits<u32>::max();
    test.u64Value = std::numeric_limits<u64>::max();
    test.i8Value = std::numeric_limits<i8>::max();
    test.i16Value = std::numeric_limits<i16>::max();
    test.i32Value = std::numeric_limits<i32>::max();
    test.i64Value = std::numeric_limits<i64>::max();
    test.f32Value = std::numeric_limits<f32>::max();
    test.f64Value = std::numeric_limits<f64>::max();

    // Serialize to YAML
    SerdeArchive sa = NewSerdeArchive(&arena, &arena, ESerdeBackend::YAML, ESerdeMode::Serialize);
    Serde(&sa, "Test", &test);

    // Convert to string for debugging
    std::string yaml_str = YAML::Dump(sa.BaseNode);
    INFO("Serialized YAML:\n" << yaml_str);

    // Deserialize from YAML
    sa.Mode = ESerdeMode::Deserialize;
    sa.BaseNode = YAML::Load(yaml_str);

    Test deserialized_test = {};
    Serde(&sa, "Test", &deserialized_test);

    REQUIRE(test.u8Value == deserialized_test.u8Value);
    REQUIRE(test.u16Value == deserialized_test.u16Value);
    REQUIRE(test.u32Value == deserialized_test.u32Value);
    REQUIRE(test.u64Value == deserialized_test.u64Value);
    REQUIRE(test.i8Value == deserialized_test.i8Value);
    REQUIRE(test.i16Value == deserialized_test.i16Value);
    REQUIRE(test.i32Value == deserialized_test.i32Value);
    REQUIRE(test.i64Value == deserialized_test.i64Value);
    REQUIRE(test.f32Value == deserialized_test.f32Value);
    REQUIRE(test.f64Value == deserialized_test.f64Value);
}

TEST_CASE("Serde", "[serde]") {
    SECTION("serialization deserialization") {
        using namespace kdk::serde_test_private;
        Arena arena = AllocateArena("TestArena"sv, 16 * MEGABYTE);
        DEFER { FreeArena(&arena); };

        // Create example data
        Foo foo;
        foo.Name = String("Example Foo");
        foo.Age = 25;
        foo.Height = 1.75f;
        foo.Ints = NewDynArray<int>(&arena);
        foo.Ints.Push(1);
        foo.Ints.Push(2);
        foo.Ints.Push(3);

        // Add some bars
        Bar bar1;
        bar1.Name = String("Bar One");
        bar1.Transform.Position = {1.0f, 2.0f, 3.0f};
        bar1.Transform.Rotation = {0.0f, 0.0f, 0.0f, 1.0f};
        bar1.Transform.Scale = Vec3(1.0f);
        bar1.Addresses = NewDynArray<String>(&arena);
        bar1.Addresses.Push(String("Address1"));
        bar1.Addresses.Push(String(R"(This is a long string
that spans multiple lines.
It preserves newlines and special characters.)"));
        bar1.FixedStrings = {};
        bar1.FixedStrings.Push({"Test1"});
        bar1.FixedStrings.Push({"Test2"});
        bar1.FixedStrings.Push({"Test3"});
        bar1.Positions = NewDynArray<Vec3>(&arena);
        bar1.Positions.Push(Vec3(1, 0, 0));
        bar1.Positions.Push(Vec3(0, 1, 0));
        bar1.Positions.Push(Vec3(0, 0, 1));

        Bar bar2 = bar1;
        bar2.Name = String("Bar Two");
        bar2.FixedStrings = {};
        bar2.FixedStrings.Push({"TestBar2"});

        foo.Bars = NewDynArray<Bar>(&arena);
        foo.Bars.Push(bar1);
        foo.Bars.Push(bar2);

        // Serialize to YAML
        SerdeArchive sa =
            NewSerdeArchive(&arena, &arena, ESerdeBackend::YAML, ESerdeMode::Serialize);
        Serde(&sa, "Foo", &foo);

        // Convert to string for debugging
        std::string yaml_str = YAML::Dump(sa.BaseNode);
        INFO("Serialized YAML:\n" << yaml_str);

        // Deserialize from YAML
        sa.Mode = ESerdeMode::Deserialize;
        sa.BaseNode = YAML::Load(yaml_str);

        Foo deserialized_foo = {};
        Serde(&sa, "Foo", &deserialized_foo);

        // Verify the deserialized data matches the original
        REQUIRE(deserialized_foo.Name == foo.Name);
        REQUIRE(deserialized_foo.Age == foo.Age);
        REQUIRE(deserialized_foo.Height == foo.Height);

        // Check Ints array
        REQUIRE(deserialized_foo.Ints.Size == foo.Ints.Size);
        for (i32 i = 0; i < foo.Ints.Size; ++i) {
            REQUIRE(deserialized_foo.Ints[i] == foo.Ints[i]);
        }

        // Check Bars array
        REQUIRE(deserialized_foo.Bars.Size == foo.Bars.Size);
        for (i32 i = 0; i < foo.Bars.Size; ++i) {
            const Bar& orig_bar = foo.Bars[i];
            const Bar& deser_bar = deserialized_foo.Bars[i];

            REQUIRE(deser_bar.Name == orig_bar.Name);

            // Check Transform
            REQUIRE(deser_bar.Transform.Position == orig_bar.Transform.Position);
            REQUIRE(deser_bar.Transform.Rotation == orig_bar.Transform.Rotation);
            REQUIRE(deser_bar.Transform.Scale == orig_bar.Transform.Scale);

            // Check Addresses
            REQUIRE(deser_bar.Addresses.Size == orig_bar.Addresses.Size);
            for (i32 j = 0; j < orig_bar.Addresses.Size; ++j) {
                REQUIRE(deser_bar.Addresses[j] == orig_bar.Addresses[j]);
            }

            // Check FixedStrings
            REQUIRE(deser_bar.FixedStrings.Size == orig_bar.FixedStrings.Size);
            for (i32 j = 0; j < orig_bar.FixedStrings.Size; ++j) {
                REQUIRE(deser_bar.FixedStrings[j] == orig_bar.FixedStrings[j]);
            }

            // Check Positions
            REQUIRE(deser_bar.Positions.Size == orig_bar.Positions.Size);
            for (i32 j = 0; j < orig_bar.Positions.Size; ++j) {
                REQUIRE(deser_bar.Positions[j] == orig_bar.Positions[j]);
            }
        }
    }
}

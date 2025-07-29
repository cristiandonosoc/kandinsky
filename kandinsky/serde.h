#pragma once

#include <kandinsky/color.h>
#include <kandinsky/container.h>
#include <kandinsky/game/entity_definitions.h>
#include <kandinsky/math.h>
#include <kandinsky/memory.h>
#include <kandinsky/string.h>

#include <yaml-cpp/yaml.h>

namespace kdk {

enum class ESerdeBackend : u8 {
    Invalid = 0,
    YAML,
};

enum class ESerdeMode : u8 {
    Invalid = 0,
    Serialize,
    Deserialize,
};

struct SerdeArchive {
    ESerdeBackend Backend = ESerdeBackend::Invalid;
    ESerdeMode Mode = ESerdeMode::Invalid;

    Arena* Arena = nullptr;

    YAML::Node BaseNode = {};
    YAML::Node* CurrentNode = nullptr;
};
bool IsValid(const SerdeArchive& sa);
SerdeArchive NewSerdeArchive(Arena* arena, ESerdeBackend backend, ESerdeMode mode);

namespace serde {

template <typename T>
void SerdeYaml(SerdeArchive* sa, const char* name, T& t) {
    auto* prev = sa->CurrentNode ? sa->CurrentNode : &sa->BaseNode;

    if (sa->Mode == ESerdeMode::Serialize) {
        YAML::Node node;
        sa->CurrentNode = &node;
        Serialize(sa, t);

        (*prev)[name] = std::move(node);
        sa->CurrentNode = prev;

    } else {
        if (const auto& node = (*prev)[name]; node.IsDefined()) {
            sa->CurrentNode = const_cast<YAML::Node*>(&node);
            Serialize(sa, t);
            sa->CurrentNode = prev;
        } else {
            t = {};
        }
    }
}

template <>
void SerdeYaml<String>(SerdeArchive* sa, const char* name, String& value);

template <>
void SerdeYaml<i32>(SerdeArchive* sa, const char* name, int& value);

template <>
void SerdeYaml<float>(SerdeArchive* sa, const char* name, float& value);

template <>
void SerdeYaml<Vec2>(SerdeArchive* sa, const char* name, Vec2& value);

template <>
void SerdeYaml<Vec3>(SerdeArchive* sa, const char* name, Vec3& value);

template <>
void SerdeYaml<Vec4>(SerdeArchive* sa, const char* name, Vec4& value);

template <>
void SerdeYaml<UVec2>(SerdeArchive* sa, const char* name, UVec2& value);

template <>
void SerdeYaml<UVec3>(SerdeArchive* sa, const char* name, UVec3& value);

template <>
void SerdeYaml<UVec4>(SerdeArchive* sa, const char* name, UVec4& value);

template <>
void SerdeYaml<Entity>(SerdeArchive* sa, const char* name, Entity& value);

template <>
void SerdeYaml<Color32>(SerdeArchive* sa, const char* name, Color32& value);

template <>
void SerdeYaml<Quat>(SerdeArchive* sa, const char* name, Quat& value);

template <>
void SerdeYaml<Transform>(SerdeArchive* sa, const char* name, Transform& value);

template <typename T>
concept HasInlineSerialization =
    std::is_same_v<T, Vec2> || std::is_same_v<T, Vec3> || std::is_same_v<T, Vec4> ||
    std::is_same_v<T, UVec2> || std::is_same_v<T, UVec3> || std::is_same_v<T, UVec4> ||
    std::is_same_v<T, Quat>;

void SerdeYamlInline(YAML::Node& node, Vec2& value);
void SerdeYamlInline(YAML::Node& node, Vec3& value);
void SerdeYamlInline(YAML::Node& node, Vec4& value);
void SerdeYamlInline(YAML::Node& node, UVec2& value);
void SerdeYamlInline(YAML::Node& node, UVec3& value);
void SerdeYamlInline(YAML::Node& node, UVec4& value);
void SerdeYamlInline(YAML::Node& node, Quat& value);
void SerdeYamlInline(YAML::Node& node, Entity& value);

template <typename T>
void SerdeYaml(SerdeArchive* sa, const char* name, DynArray<T>& values) {
    if (sa->Mode == ESerdeMode::Serialize) {
        auto* prev = sa->CurrentNode;
        YAML::Node array_node = YAML::Node(YAML::NodeType::Sequence);

        for (u32 i = 0; i < values.Size; i++) {
            auto& value = values[i];
            if constexpr (std::is_arithmetic_v<T>) {
                array_node.push_back(value);
            } else if constexpr (std::is_same_v<T, String>) {
                array_node.push_back(value.Str());
            } else if constexpr (HasInlineSerialization<T>) {
                YAML::Node node;
                SerdeYamlInline(node, value);
                array_node.push_back(std::move(node));
            } else {
                YAML::Node node;
                sa->CurrentNode = &node;
                Serialize(sa, value);
                array_node.push_back(std::move(node));
            }
        }

        (*prev)[name] = std::move(array_node);
        sa->CurrentNode = prev;
    } else {
        values.Clear();
        if (const auto& node = (*sa->CurrentNode)[name]; node.IsDefined()) {
            ASSERT(node.IsSequence());
            values.Reserve(sa->Arena, node.size());

            for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
                if constexpr (std::is_arithmetic_v<T>) {
                    values.Push(sa->Arena, it->as<T>());
                } else if constexpr (std::is_same_v<T, String>) {
                    const std::string& str = it->as<std::string>();
                    const char* interned =
                        InternStringToArena(sa->Arena, str.c_str(), str.length());
                    values.Push(sa->Arena, String(interned, str.length()));
                } else if constexpr (HasInlineSerialization<T>) {
                    T value;
                    if constexpr (std::is_same_v<T, Vec3>) {
                        value.x = (*it)["x"].as<float>();
                        value.y = (*it)["y"].as<float>();
                        value.z = (*it)["z"].as<float>();
                    } else if constexpr (std::is_same_v<T, Quat>) {
                        value.x = (*it)["x"].as<float>();
                        value.y = (*it)["y"].as<float>();
                        value.z = (*it)["z"].as<float>();
                        value.w = (*it)["w"].as<float>();
                    }
                    values.Push(sa->Arena, value);
                } else {
                    T value{};
                    auto* prev = sa->CurrentNode;
                    const YAML::Node& child = *it;
                    sa->CurrentNode = const_cast<YAML::Node*>(&child);
                    Serialize(sa, value);
                    sa->CurrentNode = prev;
                    values.Push(sa->Arena, value);
                }
            }
        }
    }
}

template <typename T, u32 N>
void SerdeYaml(SerdeArchive* sa, const char* name, FixedArray<T, N>& values) {
    if (sa->Mode == ESerdeMode::Serialize) {
        auto* prev = sa->CurrentNode;
        YAML::Node array_node = YAML::Node(YAML::NodeType::Sequence);

        for (u32 i = 0; i < values.Size; i++) {
            auto& value = values[i];
            if constexpr (std::is_arithmetic_v<T>) {
                array_node.push_back(value);
            } else if constexpr (std::is_same_v<T, String>) {
                array_node.push_back(value.Str());
            } else if constexpr (HasInlineSerialization<T>) {
                YAML::Node node;
                SerdeYamlInline(node, value);
                array_node.push_back(std::move(node));
            } else {
                YAML::Node node;
                sa->CurrentNode = &node;
                Serialize(sa, value);
                array_node.push_back(std::move(node));
            }
        }

        (*prev)[name] = std::move(array_node);
        sa->CurrentNode = prev;
    } else {
        values.Clear();
        if (const auto& node = (*sa->CurrentNode)[name]; node.IsDefined()) {
            ASSERT(node.IsSequence());

            for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
                if constexpr (std::is_arithmetic_v<T>) {
                    values.Push(it->as<T>());
                } else if constexpr (std::is_same_v<T, String>) {
                    const std::string& str = it->as<std::string>();
                    const char* interned =
                        InternStringToArena(sa->Arena, str.c_str(), str.length());
                    values.Push(String(interned, str.length()));
                } else if constexpr (HasInlineSerialization<T>) {
                    T value;
                    if constexpr (std::is_same_v<T, Vec3>) {
                        value.x = (*it)["x"].as<float>();
                        value.y = (*it)["y"].as<float>();
                        value.z = (*it)["z"].as<float>();
                    } else if constexpr (std::is_same_v<T, Quat>) {
                        value.x = (*it)["x"].as<float>();
                        value.y = (*it)["y"].as<float>();
                        value.z = (*it)["z"].as<float>();
                        value.w = (*it)["w"].as<float>();
                    }
                    values.Push(value);
                } else {
                    T value{};
                    auto* prev = sa->CurrentNode;
                    const YAML::Node& child = *it;
                    sa->CurrentNode = const_cast<YAML::Node*>(&child);
                    Serialize(sa, value);
                    sa->CurrentNode = prev;
                    values.Push(value);
                }
            }
        }
    }
}

}  // namespace serde

#define SERDE(sa, owner, member) ::kdk::Serde(sa, #member, owner.member)

template <typename T>
void Serde(SerdeArchive* sa, const char* name, T& t) {
    switch (sa->Backend) {
        case ESerdeBackend::Invalid: ASSERT(false); break;
        case ESerdeBackend::YAML: serde::SerdeYaml(sa, name, t); break;
    }
}

}  // namespace kdk

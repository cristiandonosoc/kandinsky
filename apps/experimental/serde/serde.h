#pragma once

#include <kandinsky/container.h>
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

    YAML::Node node;
    sa->CurrentNode = &node;
    Serialize(sa, t);

    (*prev)[name] = std::move(node);
    sa->CurrentNode = prev;
}

template <>
void SerdeYaml<String>(SerdeArchive* sa, const char* name, String& value);

template <>
void SerdeYaml<int>(SerdeArchive* sa, const char* name, int& value);

template <>
void SerdeYaml<float>(SerdeArchive* sa, const char* name, float& value);

template <>
void SerdeYaml<Vec3>(SerdeArchive* sa, const char* name, Vec3& value);

template <>
void SerdeYaml<Quat>(SerdeArchive* sa, const char* name, Quat& value);

template <>
void SerdeYaml<Transform>(SerdeArchive* sa, const char* name, Transform& value);

template <typename T>
concept HasInlineSerialization = std::is_same_v<T, Vec3> || std::is_same_v<T, Quat>;

void SerdeYamlInline(YAML::Node& node, Vec3& value);
void SerdeYamlInline(YAML::Node& node, Quat& value);

template <typename T>
void SerdeYaml(SerdeArchive* sa, const char* name, DynArray<T>& values) {
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

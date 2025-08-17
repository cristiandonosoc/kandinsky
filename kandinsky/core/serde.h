#pragma once

#include <kandinsky/core/color.h>
#include <kandinsky/core/container.h>
#include <kandinsky/core/math.h>
#include <kandinsky/core/memory.h>
#include <kandinsky/core/string.h>

#include <yaml-cpp/yaml.h>
#include <limits>

namespace kdk {

// This is macro helper to serialize/deserialize any struct.
#define SERDE(sa, owner, member) ::kdk::Serde(sa, #member, &owner->member)

enum class ESerdeBackend : u8 {
    Invalid = 0,
    YAML,
};

enum class ESerdeMode : u8 {
    Invalid = 0,
    Serialize,
    Deserialize,
};

enum class ESerdeErrorMode : u8 {
    Invalid = 0,
    Warn = 1,
    Stop = 2,
};

struct SerdeArchive {
    ESerdeBackend Backend = ESerdeBackend::Invalid;
    ESerdeMode Mode = ESerdeMode::Invalid;
    ESerdeErrorMode ErrorMode = ESerdeErrorMode::Stop;

    Arena* TargetArena = nullptr;
    Arena* TempArena = nullptr;

    YAML::Node BaseNode = {};
    YAML::Node* CurrentNode = nullptr;

    // Opaque context pointer that can be used by serialize functions.
    // TODO(cdc): This should be a stack, so that contexts can be pushed and popped.
    void* Context = nullptr;

    FixedArray<String, 128> Errors;
};
bool IsValid(const SerdeArchive& sa);
SerdeArchive NewSerdeArchive(Arena* arena,
                             Arena* temp_arena,
                             ESerdeBackend backend,
                             ESerdeMode mode);

void Load(SerdeArchive* ar, std::span<u8> data);

// Returns whether application should continue or not.
bool AddError(SerdeArchive* sa, String error);

String GetSerializedString(Arena* arena, const SerdeArchive& sa);

namespace serde {

template <typename T>
void SerdeYaml(SerdeArchive* sa, const char* name, T* t) {
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
void SerdeYaml<String>(SerdeArchive* sa, const char* name, String* value);

template <u64 CAPACITY>
void SerdeYaml(SerdeArchive* sa, const char* name, FixedString<CAPACITY>* value) {
    if (sa->Mode == ESerdeMode::Serialize) {
        (*sa->CurrentNode)[name] = value->Str();
    } else {
        if (const auto& node = (*sa->CurrentNode)[name]; node.IsDefined()) {
            const std::string& str = node.as<std::string>();
            if (str.size() >= CAPACITY) {
                bool should_continue =
                    AddError(sa,
                             Printf(sa->TempArena,
                                    "FixedString overflow for key '%s' (CAPACITY: %llu)",
                                    name,
                                    CAPACITY));
                if (!should_continue) {
                    return;
                }
            }
            value->Set(String(str.c_str(), str.length()));
        } else {
            *value = {};
        }
    }
}

// clang-format off
template <typename T>
concept IsImmediateType=
    std::is_same_v<T, u8>  || std::is_same_v<T, i8>   ||
	std::is_same_v<T, u16> || std::is_same_v<T, i16>  ||
    std::is_same_v<T, u32> || std::is_same_v<T, i32>  ||
	std::is_same_v<T, u64> || std::is_same_v<T, i64>  ||
	std::is_same_v<T, f32> || std::is_same_v<T, f64>;
// clang-format on

template <typename T>
    requires IsImmediateType<T>
void SerdeYaml(SerdeArchive* sa, const char* name, T* value) {
    // TODO(cdc): Use a better yaml library.
    //            We have to encode u8 and i8 as u16 and i16 because otherwise they get encoded as
    //            chars and the decoding goes wrong.
    if (sa->Mode == ESerdeMode::Serialize) {
        if constexpr (std::is_same_v<T, u8>) {
            (*sa->CurrentNode)[name] = (u16)(*value);
        } else if constexpr (std::is_same_v<T, i8>) {
            (*sa->CurrentNode)[name] = (i16)(*value);
        } else {
            (*sa->CurrentNode)[name] = *value;
        }
    } else {
        if (const auto& node = (*sa->CurrentNode)[name]; node.IsDefined()) {
            if constexpr (std::is_same_v<T, u8>) {
                u16 temp = node.as<u16>();
                ASSERT(temp <= std::numeric_limits<u8>::max());
                *value = (u8)temp;
            } else if constexpr (std::is_same_v<T, i8>) {
                i16 temp = node.as<i16>();
                ASSERT(temp <= std::numeric_limits<i8>::max() &&
                       temp >= std::numeric_limits<i8>::min());
                *value = (i8)temp;
            } else {
                *value = node.as<T>();
            }

            *value = node.as<T>();
        } else {
            *value = 0;
        }
    }
}

template <>
void SerdeYaml<Vec2>(SerdeArchive* sa, const char* name, Vec2* value);

template <>
void SerdeYaml<Vec3>(SerdeArchive* sa, const char* name, Vec3* value);

template <>
void SerdeYaml<Vec4>(SerdeArchive* sa, const char* name, Vec4* value);

template <>
void SerdeYaml<UVec2>(SerdeArchive* sa, const char* name, UVec2* value);

template <>
void SerdeYaml<UVec3>(SerdeArchive* sa, const char* name, UVec3* value);

template <>
void SerdeYaml<UVec4>(SerdeArchive* sa, const char* name, UVec4* value);

template <>
void SerdeYaml<Color32>(SerdeArchive* sa, const char* name, Color32* value);

template <>
void SerdeYaml<Quat>(SerdeArchive* sa, const char* name, Quat* value);

template <>
void SerdeYaml<Transform>(SerdeArchive* sa, const char* name, Transform* value);

template <typename T>
concept HasInlineSerialization =
    std::is_same_v<T, Vec2> || std::is_same_v<T, Vec3> || std::is_same_v<T, Vec4> ||
    std::is_same_v<T, UVec2> || std::is_same_v<T, UVec3> || std::is_same_v<T, UVec4> ||
    std::is_same_v<T, Quat>;

// Type trait to detect FixedString<CAPACITY>
template <typename T>
struct IsFixedStringTrait : std::false_type {};

template <u64 CAPACITY>
struct IsFixedStringTrait<FixedString<CAPACITY>> : std::true_type {};

void SerdeYamlInline(YAML::Node& node, Vec2* value);
void SerdeYamlInline(YAML::Node& node, Vec3* value);
void SerdeYamlInline(YAML::Node& node, Vec4* value);
void SerdeYamlInline(YAML::Node& node, UVec2* value);
void SerdeYamlInline(YAML::Node& node, UVec3* value);
void SerdeYamlInline(YAML::Node& node, UVec4* value);
void SerdeYamlInline(YAML::Node& node, Quat* value);

template <typename T>
void SerdeYaml(SerdeArchive* sa, const char* name, DynArray<T>* values) {
    if (sa->Mode == ESerdeMode::Serialize) {
        auto* prev = sa->CurrentNode;
        YAML::Node array_node = YAML::Node(YAML::NodeType::Sequence);

        for (u32 i = 0; i < values->Size; i++) {
            auto& value = values->At(i);
            if constexpr (std::is_arithmetic_v<T>) {
                array_node.push_back(value);
            } else if constexpr (std::is_same_v<T, String>) {
                array_node.push_back(value.Str());
            } else if constexpr (IsFixedStringTrait<T>::value) {
                array_node.push_back(value.Str());
            } else if constexpr (HasInlineSerialization<T>) {
                YAML::Node node;
                SerdeYamlInline(node, &value);
                array_node.push_back(std::move(node));
            } else {
                YAML::Node node;
                sa->CurrentNode = &node;
                Serialize(sa, &value);
                array_node.push_back(std::move(node));
            }
        }

        (*prev)[name] = std::move(array_node);
        sa->CurrentNode = prev;
    } else {
        values->Clear();
        if (const auto& node = (*sa->CurrentNode)[name]; node.IsDefined()) {
            ASSERT(node.IsSequence());
            values->Reserve(sa->TargetArena, (u32)node.size());

            for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
                if constexpr (std::is_arithmetic_v<T>) {
                    values->Push(sa->TargetArena, it->as<T>());
                } else if constexpr (std::is_same_v<T, String>) {
                    const std::string& str = it->as<std::string>();
                    String interned =
                        InternStringToArena(sa->TargetArena, str.c_str(), str.length());
                    values->Push(sa->TargetArena, interned);
                } else if constexpr (IsFixedStringTrait<T>::value) {
                    const std::string& str = it->as<std::string>();
                    if (str.size() >= T::kCapacity) {
                        bool should_continue =
                            AddError(sa,
                                     Printf(sa->TempArena,
                                            "FixedString overflow for key '%s' (CAPACITY: %llu)",
                                            name,
                                            T::kCapacity));
                        if (!should_continue) {
                            return;
                        }
                    }
                    values->Push(sa->TargetArena, {String(str.c_str(), str.length())});
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
                    values->Push(sa->TargetArena, value);
                } else {
                    T value{};
                    auto* prev = sa->CurrentNode;
                    const YAML::Node& child = *it;
                    sa->CurrentNode = const_cast<YAML::Node*>(&child);
                    Serialize(sa, &value);
                    sa->CurrentNode = prev;
                    values->Push(sa->TargetArena, value);
                }
            }
        }
    }
}

template <typename T, u32 N>
void SerdeYaml(SerdeArchive* sa, const char* name, FixedArray<T, N>* values) {
    if (sa->Mode == ESerdeMode::Serialize) {
        auto* prev = sa->CurrentNode;
        YAML::Node array_node = YAML::Node(YAML::NodeType::Sequence);

        for (u32 i = 0; i < values->Size; i++) {
            auto& value = values->At(i);
            if constexpr (std::is_arithmetic_v<T>) {
                array_node.push_back(value);
            } else if constexpr (std::is_same_v<T, String>) {
                array_node.push_back(value.Str());
            } else if constexpr (IsFixedStringTrait<T>::value) {
                array_node.push_back(value.Str());
            } else if constexpr (HasInlineSerialization<T>) {
                YAML::Node node;
                SerdeYamlInline(node, &value);
                array_node.push_back(std::move(node));
            } else {
                YAML::Node node;
                sa->CurrentNode = &node;
                Serialize(sa, &value);
                array_node.push_back(std::move(node));
            }
        }

        (*prev)[name] = std::move(array_node);
        sa->CurrentNode = prev;
    } else {
        values->Clear();
        if (const auto& node = (*sa->CurrentNode)[name]; node.IsDefined()) {
            ASSERT(node.IsSequence());

            for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
                if constexpr (std::is_arithmetic_v<T>) {
                    values->Push(it->as<T>());
                } else if constexpr (std::is_same_v<T, String>) {
                    const std::string& str = it->as<std::string>();
                    String interned =
                        InternStringToArena(sa->TargetArena, str.c_str(), str.length());
                    values->Push(interned);
                } else if constexpr (IsFixedStringTrait<T>::value) {
                    const std::string& str = it->as<std::string>();
                    if (str.size() >= T::kCapacity) {
                        bool should_continue =
                            AddError(sa,
                                     Printf(sa->TempArena,
                                            "FixedString overflow for key '%s' (CAPACITY: %llu)",
                                            name,
                                            T::kCapacity));
                        if (!should_continue) {
                            return;
                        }
                    }
                    values->Push({String(str.c_str(), str.length())});
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
                    values->Push(value);
                } else {
                    T value{};
                    auto* prev = sa->CurrentNode;
                    const YAML::Node& child = *it;
                    sa->CurrentNode = const_cast<YAML::Node*>(&child);
                    Serialize(sa, value);
                    sa->CurrentNode = prev;
                    values->Push(value);
                }
            }
        }
    }
}

}  // namespace serde

template <typename T>
void Serde(SerdeArchive* sa, const char* name, T* t) {
    switch (sa->Backend) {
        case ESerdeBackend::Invalid: ASSERT(false); break;
        case ESerdeBackend::YAML: serde::SerdeYaml(sa, name, t); break;
    }
}

}  // namespace kdk

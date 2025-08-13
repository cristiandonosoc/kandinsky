#include <kandinsky/core/serde.h>

#include <kandinsky/core/container.h>
#include <kandinsky/core/math.h>
#include <kandinsky/core/memory.h>
#include <kandinsky/core/string.h>

#include <SDL3/SDL_Log.h>

#include <yaml-cpp/yaml.h>

namespace kdk {

bool IsValid(const SerdeArchive& sa) {
    if (sa.Backend == ESerdeBackend::Invalid) {
        return false;
    }

    if (sa.Mode == ESerdeMode::Invalid) {
        return false;
    }

    if (sa.Arena == nullptr) {
        return false;
    }

    if (sa.CurrentNode == nullptr) {
        return false;
    }

    return true;
}

SerdeArchive NewSerdeArchive(Arena* arena, ESerdeBackend backend, ESerdeMode mode) {
    SerdeArchive sa{
        .Backend = backend,
        .Mode = mode,
        .Arena = arena,
    };
    sa.BaseNode = YAML::Node();

    return sa;
}

bool AddError(SerdeArchive* sa, String error) {
    sa->Errors.Push(error);
    if (sa->ErrorMode == ESerdeErrorMode::Warn) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Serde warning: %s", error.Str());
        return true;
    } else if (sa->ErrorMode == ESerdeErrorMode::Stop) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Serde error: %s", error.Str());
        return false;
    }

    ASSERT(false);
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Invalid serde mode");
    return false;
}

namespace serde {

template <>
void SerdeYaml<String>(SerdeArchive* sa, const char* name, String* value) {
    if (sa->Mode == ESerdeMode::Serialize) {
        (*sa->CurrentNode)[name] = value->Str();
    } else {
        if (const auto& node = (*sa->CurrentNode)[name]; node.IsDefined()) {
            const std::string& str = node.as<std::string>();
            const char* interned = InternStringToArena(sa->Arena, str.c_str(), str.length());
            *value = String(interned, str.length());
        } else {
            *value = {};
        }
    }
}

template <>
void SerdeYaml<i32>(SerdeArchive* sa, const char* name, int* value) {
    if (sa->Mode == ESerdeMode::Serialize) {
        (*sa->CurrentNode)[name] = *value;
    } else {
        if (const auto& node = (*sa->CurrentNode)[name]; node.IsDefined()) {
            *value = node.as<int>();
        } else {
            *value = 0;
        }
    }
}

template <>
void SerdeYaml<float>(SerdeArchive* sa, const char* name, float* value) {
    if (sa->Mode == ESerdeMode::Serialize) {
        (*sa->CurrentNode)[name] = *value;
    } else {
        if (const auto& node = (*sa->CurrentNode)[name]; node.IsDefined()) {
            *value = node.as<float>();
        } else {
            *value = 0.0f;
        }
    }
}

template <>
void SerdeYaml<Vec2>(SerdeArchive* sa, const char* name, Vec2* value) {
    if (sa->Mode == ESerdeMode::Serialize) {
        YAML::Node node;
        SerdeYamlInline(node, value);
        (*sa->CurrentNode)[name] = std::move(node);
    } else {
        if (const auto& node = (*sa->CurrentNode)[name]; node.IsDefined()) {
            value->x = node["x"].as<float>();
            value->y = node["y"].as<float>();
        } else {
            *value = {};
        }
    }
}

template <>
void SerdeYaml<Vec3>(SerdeArchive* sa, const char* name, Vec3* value) {
    if (sa->Mode == ESerdeMode::Serialize) {
        YAML::Node node;
        SerdeYamlInline(node, value);
        (*sa->CurrentNode)[name] = std::move(node);
    } else {
        if (const auto& node = (*sa->CurrentNode)[name]; node.IsDefined()) {
            value->x = node["x"].as<float>();
            value->y = node["y"].as<float>();
            value->z = node["z"].as<float>();
        } else {
            *value = {};
        }
    }
}

template <>
void SerdeYaml<Color32>(SerdeArchive* sa, const char* name, Color32* value) {
    if (sa->Mode == ESerdeMode::Serialize) {
        (*sa->CurrentNode)[name] = value->Bits;
    } else {
        if (const auto& node = (*sa->CurrentNode)[name]; node.IsDefined()) {
            value->Bits = node.as<u32>();
        } else {
            *value = Color32::White;
        }
    }
}

template <>
void SerdeYaml<Quat>(SerdeArchive* sa, const char* name, Quat* value) {
    if (sa->Mode == ESerdeMode::Serialize) {
        YAML::Node node;
        serde::SerdeYamlInline(node, value);
        (*sa->CurrentNode)[name] = std::move(node);
    } else {
        if (const auto& node = (*sa->CurrentNode)[name]; node.IsDefined()) {
            value->x = node["x"].as<float>();
            value->y = node["y"].as<float>();
            value->z = node["z"].as<float>();
            value->w = node["w"].as<float>();
        } else {
            *value = {};
        }
    }
}

template <>
void SerdeYaml<Vec4>(SerdeArchive* sa, const char* name, Vec4* value) {
    if (sa->Mode == ESerdeMode::Serialize) {
        YAML::Node node;
        SerdeYamlInline(node, value);
        (*sa->CurrentNode)[name] = std::move(node);
    } else {
        if (const auto& node = (*sa->CurrentNode)[name]; node.IsDefined()) {
            value->x = node["x"].as<float>();
            value->y = node["y"].as<float>();
            value->z = node["z"].as<float>();
            value->w = node["w"].as<float>();
        } else {
            *value = {};
        }
    }
}

template <>
void SerdeYaml<UVec2>(SerdeArchive* sa, const char* name, UVec2* value) {
    if (sa->Mode == ESerdeMode::Serialize) {
        YAML::Node node;
        SerdeYamlInline(node, value);
        (*sa->CurrentNode)[name] = std::move(node);
    } else {
        if (const auto& node = (*sa->CurrentNode)[name]; node.IsDefined()) {
            value->x = node["x"].as<u32>();
            value->y = node["y"].as<u32>();
        } else {
            *value = {};
        }
    }
}

template <>
void SerdeYaml<UVec3>(SerdeArchive* sa, const char* name, UVec3* value) {
    if (sa->Mode == ESerdeMode::Serialize) {
        YAML::Node node;
        SerdeYamlInline(node, value);
        (*sa->CurrentNode)[name] = std::move(node);
    } else {
        if (const auto& node = (*sa->CurrentNode)[name]; node.IsDefined()) {
            value->x = node["x"].as<u32>();
            value->y = node["y"].as<u32>();
            value->z = node["z"].as<u32>();
        } else {
            *value = {};
        }
    }
}

template <>
void SerdeYaml<UVec4>(SerdeArchive* sa, const char* name, UVec4* value) {
    if (sa->Mode == ESerdeMode::Serialize) {
        YAML::Node node;
        SerdeYamlInline(node, value);
        (*sa->CurrentNode)[name] = std::move(node);
    } else {
        if (const auto& node = (*sa->CurrentNode)[name]; node.IsDefined()) {
            value->x = node["x"].as<u32>();
            value->y = node["y"].as<u32>();
            value->z = node["z"].as<u32>();
            value->w = node["w"].as<u32>();
        } else {
            *value = {};
        }
    }
}

template <>
void SerdeYaml<Transform>(SerdeArchive* sa, const char* name, Transform* value) {
    if (sa->Mode == ESerdeMode::Serialize) {
        auto* prev = sa->CurrentNode;
        YAML::Node node;
        sa->CurrentNode = &node;
        SERDE(sa, value, Position);
        SERDE(sa, value, Rotation);
        SERDE(sa, value, Scale);
        (*prev)[name] = std::move(node);
        sa->CurrentNode = prev;
    } else {
        if (const auto& node = (*sa->CurrentNode)[name]; node.IsDefined()) {
            auto* prev = sa->CurrentNode;
            sa->CurrentNode = const_cast<YAML::Node*>(&node);
            SERDE(sa, value, Position);
            SERDE(sa, value, Rotation);
            SERDE(sa, value, Scale);
            sa->CurrentNode = prev;
        } else {
            *value = {};
        }
    }
}

void SerdeYamlInline(YAML::Node& node, Vec2* value) {
    node["x"] = value->x;
    node["y"] = value->y;
}

void SerdeYamlInline(YAML::Node& node, Vec3* value) {
    node["x"] = value->x;
    node["y"] = value->y;
    node["z"] = value->z;
}

void SerdeYamlInline(YAML::Node& node, Vec4* value) {
    node["x"] = value->x;
    node["y"] = value->y;
    node["z"] = value->z;
    node["w"] = value->w;
}

void SerdeYamlInline(YAML::Node& node, UVec2* value) {
    node["x"] = value->x;
    node["y"] = value->y;
}

void SerdeYamlInline(YAML::Node& node, UVec3* value) {
    node["x"] = value->x;
    node["y"] = value->y;
    node["z"] = value->z;
}

void SerdeYamlInline(YAML::Node& node, UVec4* value) {
    node["x"] = value->x;
    node["y"] = value->y;
    node["z"] = value->z;
    node["w"] = value->w;
}

void SerdeYamlInline(YAML::Node& node, Quat* value) {
    node["x"] = value->x;
    node["y"] = value->y;
    node["z"] = value->z;
    node["w"] = value->w;
}

}  // namespace serde

}  // namespace kdk

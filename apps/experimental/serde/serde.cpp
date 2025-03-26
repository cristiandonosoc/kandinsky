#include <experimental/serde/serde.h>

#include <kandinsky/container.h>
#include <kandinsky/math.h>
#include <kandinsky/memory.h>
#include <kandinsky/string.h>
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

namespace serde {

template <>
void SerdeYaml<String>(SerdeArchive* sa, const char* name, String& value) {
    (*sa->CurrentNode)[name] = value.Str();
}

template <>
void SerdeYaml<int>(SerdeArchive* sa, const char* name, int& value) {
    (*sa->CurrentNode)[name] = value;
}

template <>
void SerdeYaml<float>(SerdeArchive* sa, const char* name, float& value) {
    (*sa->CurrentNode)[name] = value;
}

template <>
void SerdeYaml<Vec3>(SerdeArchive* sa, const char* name, Vec3& value) {
    YAML::Node node;
    SerdeYamlInline(node, value);
    (*sa->CurrentNode)[name] = std::move(node);
}

template <>
void SerdeYaml<Quat>(SerdeArchive* sa, const char* name, Quat& value) {
    YAML::Node node;
    serde::SerdeYamlInline(node, value);
    (*sa->CurrentNode)[name] = std::move(node);
}

template <>
void SerdeYaml<Transform>(SerdeArchive* sa, const char* name, Transform& value) {
    auto* prev = sa->CurrentNode;

    YAML::Node node;
    sa->CurrentNode = &node;
    SERDE(sa, value, Position);
    SERDE(sa, value, Rotation);
    SERDE(sa, value, Scale);

    (*prev)[name] = std::move(node);
    sa->CurrentNode = prev;
}

void SerdeYamlInline(YAML::Node& node, Vec3& value) {
    node["x"] = value.x;
    node["y"] = value.y;
    node["z"] = value.z;
}

void SerdeYamlInline(YAML::Node& node, Quat& value) {
    node["x"] = value.x;
    node["y"] = value.y;
    node["z"] = value.z;
    node["w"] = value.w;
}

}  // namespace serde

}  // namespace kdk

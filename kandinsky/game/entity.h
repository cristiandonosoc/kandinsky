#pragma once

#include <kandinsky/defines.h>
#include <kandinsky/math.h>

#include <array>

namespace kdk {

struct EntityTrack;
struct SerdeArchive;

// This macro defines all entity types in the system
// Format: (enum_value, type_name, max_editor_instances, max_runtime_instances, validates)
#define ENTITY_TYPES(X)                                \
    X(Box, Box, 64, 64, false)                         \
    X(DirectionalLight, DirectionalLight, 1, 1, false) \
    X(PointLight, PointLight, 16, 16, false)           \
    X(Spotlight, Spotlight, 8, 8, false)               \
    X(Tower, Tower, 64, 64, true)                      \
    X(Spawner, Spawner, 32, 32, true)                  \
    X(Enemy, Enemy, 64, 64, false)                     \
    X(Base, Base, 1, 1, true)

enum class EEntityType : u8 {
    Invalid = 0,
#define X(enum_value, type_name, max_editor_instances, max_runtime_instances, ...) enum_value,
    ENTITY_TYPES(X)
#undef X
    COUNT,
};

const char* ToString(EEntityType entity_type);

constexpr u32 GetMaxInstances(EEntityType entity_type) {
    // clang-format off
#define X(enum_value, type_name, max_editor_instances, max_runtime_instances, ...) \
    case EEntityType::enum_value: return max_editor_instances;

    switch (entity_type) {
        case EEntityType::Invalid: ASSERT(false); return 0;
        case EEntityType::COUNT: ASSERT(false); return 0;
		ENTITY_TYPES(X)
    }

#undef X
    // clang-format on

    ASSERT(false);
    return 0;
}

#define GENERATE_ENTITY(entity)                                                     \
    kdk::Entity Entity = {};                                                        \
    static kdk::EEntityType StaticEntityType() { return kdk::EEntityType::entity; } \
    kdk::Transform& GetTransform() { return Entity.Transform; }                     \
    const kdk::Mat4& GetModelMatrix() { return Entity.M_Model; }

// Upper 8 bits: Entity type
// Rest: value.
struct EditorID {
    u64 Value = 0;

    EEntityType GetEntityType() const { return (EEntityType)((Value >> 56) & 0xFF); }
    u64 GetValue() const { return Value & 0x00FFFFFFFFFFFFFF; }
    UVec2 ToUVec2() const { return UVec2((u32)(Value & 0xFFFFFFFF), (u32)(Value >> 32)); }

    bool operator==(const EditorID& other) const { return Value == other.Value; }
};

inline bool IsValid(const EditorID& editor_id) { return editor_id.Value != 0; }
void BuildImGui(const EditorID& editor_id);
String ToString(Arena* arena, const EditorID& editor_id);

EditorID GenerateNewEditorID(EEntityType entity_type);

// EntityID are a set of two values:
// - Upper 8 bit: The entity type. This corresponds to the EEntityType value.
// - Lower 24 bit: The entity index. The entity manager tracks each entity type with a separate
//				   index.
struct InstanceID {
    u32 Index = 0;
    u16 Generation = 0;
    EEntityType EntityType = EEntityType::Invalid;
};
static_assert(sizeof(InstanceID) == 8);
void BuildImGui(const InstanceID& id);

struct Entity {
    EditorID EditorID = {};
    InstanceID InstanceID = {};
    Transform Transform = {};
    Mat4 M_Model = {};
};

void Serialize(SerdeArchive* sa, Entity& entity);
void BuildImGui(Entity* entity);

// Copies an entity from src to dst, but keeps the EntityID the same.
template <typename T>
void FillEntity(T* dst, const T& src) {
    Entity entity = dst->Entity;
    *dst = src;
    dst->Entity = std::move(entity);
}

}  // namespace kdk

#pragma once

#include <kandinsky/defines.h>
#include <kandinsky/math.h>

#include <array>

namespace kdk {

struct EntityTrack;

enum class EEntityType : u8 {
    Invalid = 0,
    Box,
    DirectionalLight,
    PointLight,
    Spotlight,
    Tower,
    COUNT,
};
const char* ToString(EEntityType entity_type);

constexpr u32 GetMaxInstances(EEntityType entity_type) {
    switch (entity_type) {
        case EEntityType::Invalid: ASSERT(false); return 0;
        case EEntityType::Box: return 64;
        case EEntityType::DirectionalLight: return 1;
        case EEntityType::PointLight: return 16;
        case EEntityType::Spotlight: return 8;
        case EEntityType::Tower: return 64;
        case EEntityType::COUNT: ASSERT(false); return 0;
    }
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
};
inline bool IsValid(const EditorID& editor_id) { return editor_id.Value != 0; }
void BuildImgui(const EditorID& editor_id);

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
void BuildImgui(const InstanceID& id);

struct Entity {
    EditorID EditorID = {};
    InstanceID InstanceID = {};
    Transform Transform = {};
    Mat4 M_Model = {};
};

// Copies an entity from src to dst, but keeps the EntityID the same.
template <typename T>
void FillEntity(T* dst, const T& src) {
    Entity entity = dst->Entity;
    *dst = src;
    dst->Entity = std::move(entity);
}

}  // namespace kdk

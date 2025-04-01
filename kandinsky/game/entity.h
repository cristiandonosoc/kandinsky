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
        case EEntityType::COUNT: ASSERT(false); return 0;
    }
}

#define GENERATE_ENTITY(entity)                                                     \
    kdk::Entity Entity = {};                                                        \
    static kdk::EEntityType StaticEntityType() { return kdk::EEntityType::entity; } \
    kdk::Transform& GetTransform() { return Entity.Transform; }                     \
    const kdk::Mat4& GetModelMatrix() { return Entity.M_Model; }

// EntityID are a set of two values:
// - Upper 8 bit: The entity type. This corresponds to the EEntityType value.
// - Lower 24 bit: The entity index. The entity manager tracks each entity type with a separate
//				   index.
struct EntityID {
    u32 ID = 0;

    EntityID() {}
    EntityID(EEntityType type, u32 index) : ID(((u32)type << 24) | (index & 0x00FFFFFF)) {}

    EEntityType GetEntityType() const { return (EEntityType)(ID >> 24); }
    u32 GetIndex() const { return ID & 0x00FFFFFF; }
};
inline bool IsValid(const EntityID& id) { return id.ID != 0; }
void BuildImgui(const EntityID& id);

struct InstanceID {
    u32 ID = 0;
    u16 Generation = 0;
    u8 _Padding = 0;
};
static_assert(sizeof(InstanceID) == 8);

struct Entity {
    InstanceID InstanceID = {};
    EntityID EntityID = {};
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

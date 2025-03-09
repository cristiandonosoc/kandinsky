#pragma once

#include <kandinsky/defines.h>
#include <kandinsky/math.h>

#include <array>

namespace kdk {

enum class EEntityType : u8 {
    Invalid = 0,
    Box,
    Model,
    Light,
    COUNT,
};
const char* ToString(EEntityType entity_type);

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

struct Entity {
    EntityID ID = {};
    void* Data = nullptr;

    EEntityType GetEntityType() const { return ID.GetEntityType(); }
};
static_assert(sizeof(Entity) == 16);

inline bool IsValid(const Entity& e) { return IsValid(e.ID); }

struct EntityTrack {
    static constexpr u32 kMaxEntityCount = 128;
    EEntityType EntityType = EEntityType::Invalid;

    std::array<Entity, kMaxEntityCount> Entities = {};
    std::array<Transform, kMaxEntityCount> Transforms = {};
    std::array<Mat4, kMaxEntityCount> ModelMatrices = {};

    u32 EntityCount = 0;
};
Entity* FindEntity(EntityTrack* track, const EntityID& id);
Transform& GetEntityTransform(EntityTrack* track, const EntityID& id);
Mat4* GetEntityModelMatrix(EntityTrack* track, const EntityID& id);

struct EntityManager {
    EntityTrack Boxes = {
        .EntityType = EEntityType::Box,
    };
    EntityTrack Models = {
        .EntityType = EEntityType::Model,
    };
    EntityTrack Lights = {
        .EntityType = EEntityType::Light,
    };

    EntityID HoverEntityID = {};
    EntityID SelectedEntityID = {};
};

template <EEntityType Type>
EntityTrack* GetEntityTrack(EntityManager* em) {
    switch (Type) {
        case EEntityType::Invalid: ASSERT(false); return nullptr;
        case EEntityType::Box: return &em->Boxes;
        case EEntityType::Model: return &em->Models;
        case EEntityType::Light: return &em->Lights;
        case EEntityType::COUNT: ASSERT(false); return nullptr;
    }

    ASSERT(false);
    return nullptr;
}

Entity* AddEntity(EntityManager* em, EEntityType type, const Transform& transform = {});

Entity* FindEntity(EntityManager* em, const EntityID& id);
Entity* GetSelectedEntity(EntityManager* em);

Transform& GetEntityTransform(EntityManager* em, const Entity& entity);
const Mat4& GetEntityModelMatrix(EntityManager* em, const Entity& entity);

}  // namespace kdk

#pragma once

#include <kandinsky/defines.h>
#include <kandinsky/math.h>

#include <array>

namespace kdk {

enum class EEntityType : u8 {
    Invalid = 0,
    Box,
    Light,
    Camera,
    COUNT,
};
const char* ToString(EEntityType entity_type);

u32 GetMaxInstances(EEntityType entity_type);

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

struct EntityTrack {
    // static constexpr u32 kMaxEntityCount = 128;
    EEntityType EntityType = EEntityType::Invalid;
    u32 EntityCount = 0;
    u32 MaxEntityCount = 0;

    void* Entities = nullptr;
    Transform* Transforms = nullptr;
    Mat4* ModelMatrices = nullptr;
};
bool IsValid(const EntityTrack& track);

void* FindEntity(EntityTrack* track, const EntityID& id);

template <typename T>
T* FindEntityTyped(EntityTrack* track, const EntityID& id) {
    return (T*)FindEntity(track, id);
}

Transform& GetEntityTransform(EntityTrack* track, const EntityID& id);
Mat4* GetEntityModelMatrix(EntityTrack* track, const EntityID& id);

struct EntityManager {
    std::array<EntityTrack, (u32)EEntityType::COUNT> EntityTracks = {};
    EntityID HoverEntityID = {};
    EntityID SelectedEntityID = {};
};

bool IsValid(const EntityManager& em);
void InitEntityManager(Arena* arena, EntityManager* em);

inline EntityTrack* GetEntityTrack(EntityManager* em, EEntityType type) {
    ASSERT(type != EEntityType::Invalid && type != EEntityType::COUNT);
    return &em->EntityTracks[(u32)type];
}

template <typename T>
T* AddEntity(EntityManager* em, const Transform& transform = {}) {
    ASSERT(T::StaticEntityType() != EEntityType::Invalid &&
           T::StaticEntityType() != EEntityType::COUNT);
    return (T*)AddEntity(em, T::StaticEntityType(), transform);
}

void* AddEntity(EntityManager* em, EEntityType type, const Transform& transform = {});

std::pair<void*, EEntityType> GetSelectedEntity(EntityManager* em);

Transform& GetEntityTransform(EntityManager* em, const EntityID& id);
const Mat4& GetEntityModelMatrix(EntityManager* em, const EntityID& id);

// TODO(cdc): Remove this eventually.
struct Box {
    static EEntityType StaticEntityType() { return EEntityType::Box; }
    EntityID ID = {};

    Vec3 Position = {};
};

}  // namespace kdk

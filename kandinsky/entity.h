#pragma once

#include <kandinsky/defines.h>
#include <kandinsky/math.h>

#include <array>

namespace kdk {

struct EntityTrack;

enum class EEntityType : u8 {
    Invalid = 0,
    Box,
    Light,
    Camera,
    COUNT,
};
const char* ToString(EEntityType entity_type);

u32 GetMaxInstances(EEntityType entity_type);

#define GENERATE_ENTITY(entity)                                                                   \
    static EEntityType StaticEntityType() { return EEntityType::entity; }                         \
    Transform& GetTransform() { return GetEntityTransform(EntityManager::Get(), EntityID); }      \
    const Mat4& GetModelMatrix() { return GetEntityModelMatrix(EntityManager::Get(), EntityID); } \
    EntityID EntityID = {};

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

// Copies an entity from src to dst, but keeps the EntityID the same.
template <typename T>
void FillEntity(T* dst, const T& src) {
	EntityID id = dst->EntityID;
	*dst = src;
	dst->EntityID = id;
}

// EntityIterator ----------------------------------------------------------------------------------

template <typename T>
struct EntityIterator {
    EntityIterator() = default;

    T& Get() { return _Entities[_Index]; }
    T& operator*() { return _Entities[_Index]; }
    T* operator->() { return &_Entities[_Index]; }

    operator bool() const { return _Index < _EntityCount; }
    void operator++() { _Index++; }
    void operator++(int) { _Index++; }

    EntityIterator(T* entities, u32 entity_count, u32 index)
        : _Entities(entities), _EntityCount(entity_count), _Index(index) {}

    T* _Entities = nullptr;
    u32 _EntityCount = 0;
    u32 _Index = 0;
};

// EntityTrack -------------------------------------------------------------------------------------

struct EntityTrack {
    EEntityType EntityType = EEntityType::Invalid;
    u32 EntityCount = 0;
    u32 MaxEntityCount = 0;

    void* Entities = nullptr;
    Transform* Transforms = nullptr;
    Mat4* ModelMatrices = nullptr;

    template <typename T>
    EntityIterator<T> GetIterator() {
        ASSERT(EntityType == T::StaticEntityType());
        return EntityIterator<T>((T*)Entities, EntityCount, 0);
    }
};
bool IsValid(const EntityTrack& track);

void* FindEntityRaw(EntityTrack* track, const EntityID& id);
template <typename T>
T* FindEntity(EntityTrack* track, const EntityID& id) {
    return (T*)FindEntityRaw(track, id);
}

Transform& GetEntityTransform(EntityTrack* track, const EntityID& id);
Mat4* GetEntityModelMatrix(EntityTrack* track, const EntityID& id);

// EntityManager -----------------------------------------------------------------------------------

struct EntityManager {
    static EntityManager* Get();
	static void Set(EntityManager* em);

    std::array<EntityTrack, (u32)EEntityType::COUNT> EntityTracks = {};
    EntityID HoverEntityID = {};
    EntityID SelectedEntityID = {};
};

bool IsValid(const EntityManager& em);
void InitEntityManager(Arena* arena, EntityManager* em);

template <typename T>
T* FindEntity(EntityManager* em, const EntityID& id) {
    return FindEntity<T>(GetEntityTrack<T>(em), id);
}
void* FindentityRaw(EntityManager* em, const EntityID& id);

template <typename T>
T* AddEntity(EntityManager* em, const Transform& transform = {}) {
    return (T*)AddEntityRaw(em, T::StaticEntityType(), transform);
}
void* AddEntityRaw(EntityManager* em, EEntityType type, const Transform& transform = {});


template <typename T>
EntityTrack* GetEntityTrack(EntityManager* em) {
    return GetEntityTrackRaw(em, T::StaticEntityType());
}
inline EntityTrack* GetEntityTrackRaw(EntityManager* em, EEntityType type) {
    ASSERT(type != EEntityType::Invalid && type != EEntityType::COUNT);
    return &em->EntityTracks[(u32)type];
}

template <typename T>
EntityIterator<T> GetEntityIterator(EntityManager* em) {
    auto* track = GetEntityTrackRaw(em, T::StaticEntityType());
    if (!track) {
        return {};
    }

    return track->template GetIterator<T>();
}

Transform& GetEntityTransform(EntityManager* em, const EntityID& id);
const Mat4& GetEntityModelMatrix(EntityManager* em, const EntityID& id);

void UpdateModelMatrices(EntityManager* em);

// TODO(cdc): Remove this eventually.
struct Box {
	GENERATE_ENTITY(Box);

    Vec3 Position = {};
};

}  // namespace kdk

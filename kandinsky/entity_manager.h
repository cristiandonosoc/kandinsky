#pragma once

#include <kandinsky/entity.h>

#include <kandinsky/gameplay/building.h>
#include <kandinsky/gameplay/enemy.h>
#include <kandinsky/gameplay/health_component.h>
#include <kandinsky/gameplay/player.h>
#include <kandinsky/gameplay/projectile.h>
#include <kandinsky/gameplay/spawner.h>
#include <kandinsky/graphics/light.h>
#include <kandinsky/graphics/model.h>

namespace kdk {

struct Scene;

template <typename T, i32 SIZE>
struct EntityComponentHolder {
    static constexpr i32 kMaxComponents = SIZE;

    Array<EntityComponentIndex, kMaxEntities> EntityToComponent;
    Array<EntityID, SIZE> ComponentToEntity;
    Array<T, SIZE> Components = {};
    FixedVector<EntityID, SIZE> ActiveEntities;
    EntityManager* Owner = nullptr;
    EntityComponentIndex NextComponent = 0;
    i32 ComponentCount = 0;

    void Init(EntityManager* em);
    void Shutdown() {}
    void Recalculate(EntityManager* em);

    std::pair<EntityComponentIndex, T*> AddEntity(EntityID id,
                                                  Entity* entity,
                                                  const T* initial_values = nullptr);
    std::pair<EntityComponentIndex, T*> GetEntity(EntityID id);
    void RemoveEntity(EntityID id);
};

struct EntityTypeWrapper {
    // Entity meant just for internal update purposes.
    struct OpaqueEntity {
        Entity* OwningEntity = nullptr;
    };

    union {
        OpaqueEntity _OpaqueEntity;
#define X(ENUM_NAME, STRUCT_NAME, ...) STRUCT_NAME ENUM_NAME##_Entity;
        ENTITY_TYPES(X)
#undef X
    };
};

struct EntityManager {
    Scene* _OwnerScene = nullptr;
    i32 NextIndex = 0;
    i32 EntityCount = 0;

    struct {
        Array<u8, kMaxEntities> Generations = {};
        Array<EntitySignature, kMaxEntities> Signatures = {};
        Array<Entity, kMaxEntities> Entities = {};
        Array<EntityTypeWrapper, kMaxEntities> EntityTypeWrappers = {};
        Array<Mat4, kMaxEntities> ModelMatrices = {};

#define X(ENUM_NAME, STRUCT_NAME, MAX_COUNT, ...) \
    FixedVector<EntityID, MAX_COUNT> Entity_##ENUM_NAME##_Alive = {};
        ENTITY_TYPES(X)
#undef X
    } EntityData;

    // Create the component arrays.
#define X(ENUM_NAME, STRUCT_NAME, MAX_COUNT, ...) \
    EntityComponentHolder<STRUCT_NAME, MAX_COUNT> ENUM_NAME##ComponentHolder;

    COMPONENT_TYPES(X)
#undef X
};

struct InitEntityManagerOptions {
    // Whether to calcualte the next index chain.
    // Normally this is not done when you expect to insert hardcoded indices in specific places,
    // like when deserializing from a scene.
    bool Recalculate : 1 = true;
};

void Init(EntityManager* em, const InitEntityManagerOptions& options = {});
void Shutdown(EntityManager* em);

void Start(EntityManager* em);
void Update(EntityManager* em, float dt);

// Recalculate is a very specific function that is meant to "fix" certain aspects of the manager
// that might be out of whack. In particularly, the next index chain is not serialized, so it has to
// be recalculated.
void Recalculate(EntityManager* em);

void CalculateModelMatrices(EntityManager* em);

void Serialize(SerdeArchive* sa, EntityManager* em);

// ARCHETYPE ---------------------------------------------------------------------------------------

struct ArchetypeComponentHolder {
#define X(ENUM_NAME, STRUCT_NAME, MAX_COUNT, ...) Optional<STRUCT_NAME> ENUM_NAME##Component = {};
    COMPONENT_TYPES(X)
#undef X
};

struct Archetype {
    FixedString<128> Name = {};
    EEntityType EntityType = EEntityType::Invalid;
    EntityFlags EntityFlags = {};
    EntityTypeWrapper TypedEntity = {};
    // ArchetypeComponentHolder ComponentHolder = {};
};
void Serialize(SerdeArchive* sa, Archetype* archetype);

struct ArchetypeManager {
    FixedVector<Archetype, 1024> Archetypes;
};

void Serialize(SerdeArchive* sa, ArchetypeManager* am);

bool LoadArchetypes(PlatformState* ps, const String& path);
}  // namespace kdk

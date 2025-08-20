#pragma once

#include <kandinsky/entity.h>

#include <kandinsky/graphics/light.h>
#include <kandinsky/graphics/model.h>

namespace kdk {

template <typename T, i32 SIZE>
struct EntityComponentHolder {
    static constexpr i32 kMaxComponents = SIZE;

    std::array<EntityComponentIndex, kMaxEntities> EntityToComponent;
    std::array<EntityID, SIZE> ComponentToEntity;
    std::array<T, SIZE> Components = {};
    FixedArray<EntityID, SIZE> ActiveEntities;
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

struct EntityComponentSet {
    // Create the component arrays.
#define X(component_enum_name, component_struct_name, component_max_count, ...) \
    EntityComponentHolder<component_struct_name, component_max_count>           \
        component_enum_name##ComponentHolder;

    ECS_COMPONENT_TYPES(X)
#undef X
};

struct EntityManager {
    i32 NextIndex = 0;
    i32 EntityCount = 0;
    std::array<u8, kMaxEntities> Generations = {};
    std::array<EntitySignature, kMaxEntities> Signatures = {};
    std::array<Entity, kMaxEntities> Entities = {};

    EntityComponentSet Components = {};
};

struct InitEntityManagerOptions {
    // Whether to calcualte the next index chain.
    // Normally this is not done when you expect to insert hardcoded indices in specific places,
    // like when deserializing from a scene.
    bool Recalculate : 1 = true;
};

void Init(EntityManager* em, const InitEntityManagerOptions& options = {});
void Shutdown(EntityManager* em);

// Recalculate is a very specific function that is meant to "fix" certain aspects of the manager
// that might be out of whack. In particularly, the next index chain is not serialized, so it has to
// be recalculated.
void Recalculate(EntityManager* em);

}  // namespace kdk

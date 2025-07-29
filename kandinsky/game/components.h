#pragma once

#include <kandinsky/game/entity_definitions.h>
#include <kandinsky/math.h>

#include <SDL3/SDL.h>

namespace kdk {

const char* ToString(EEntityComponentType component_type);

template <typename T, i32 SIZE>
struct EntityComponentHolder {
    static constexpr i32 kMaxComponents = SIZE;

    std::array<EntityComponentIndex, kMaxEntities> EntityToComponent;
    std::array<Entity, SIZE> ComponentToEntity;
    std::array<T, SIZE> Components = {};
    EntityComponentIndex NextComponent = 0;
    i32 ComponentCount = 0;

    void Init();
    void Shutdown() {}

    T* AddEntity(Entity entity);
    void RemoveEntity(Entity entity);
};

// COMPONENTS --------------------------------------------------------------------------------------

struct TestComponent {
    GENERATE_COMPONENT(Test);

    i32 Value = 0;
};

struct Test2Component {
    GENERATE_COMPONENT(Test2);

    String Name = {};
    Transform Transform = {};
};

// TEMPLATE IMPLEMENTATION -------------------------------------------------------------------------

template <typename T, i32 SIZE>
void EntityComponentHolder<T, SIZE>::Init() {
    EEntityComponentType component_type = T::kComponentType;

    for (i32 i = 0; i < SIZE; i++) {
        Components[i] = {};
    }

    for (auto& elem : EntityToComponent) {
        elem = NONE;
    }

    // Empty components point to the *next* empty component.
    // The last component points to NONE.
    for (i32 i = 0; i < SIZE; i++) {
        ComponentToEntity[i] = i + 1;
    }
    ComponentToEntity.back() = NONE;

    SDL_Log("Initialized ComponentHolder: %s\n", ToString(component_type));
}

template <typename T, i32 SIZE>
T* EntityComponentHolder<T, SIZE>::AddEntity(Entity entity) {
    i32 entity_index = GetEntityIndex(entity);

    ASSERT(entity_index >= 0 && entity_index < kMaxEntities);
    ASSERT(ComponentCount < SIZE);
    ASSERT(EntityToComponent[entity_index] == NONE);

    // Find the next empty entity.
    EntityComponentIndex component_index = NextComponent;
    ASSERT(component_index != NONE);

    // Update the translation arrays.
    NextComponent = ComponentToEntity[component_index];
    ComponentToEntity[component_index] = entity;
    EntityToComponent[entity_index] = component_index;

    ComponentCount++;

    // Reset the component.
    T* component = &Components[component_index];
    *component = {};

    return component;
}

template <typename T, i32 SIZE>
void EntityComponentHolder<T, SIZE>::RemoveEntity(Entity entity) {
    i32 entity_index = GetEntityIndex(entity);

    ASSERT(entity_index >= 0 && entity_index < kMaxEntities);
    ASSERT(ComponentCount > 0);

    // Get the component index for this entity
    EntityComponentIndex component_index = EntityToComponent[entity_index];
    ASSERT(component_index != NONE);

    // Update the translation arrays.
    EntityToComponent[entity_index] = NONE;
    ComponentToEntity[component_index] = NextComponent;
    NextComponent = component_index;

    ComponentCount--;
}

}  // namespace kdk

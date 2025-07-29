#pragma once

#include <kandinsky/ecs/ecs_definitions.h>
#include <kandinsky/math.h>

#include <SDL3/SDL.h>

namespace kdk {

// X macro for defining component types.
// Format: (component_enum_name, component_struct_name, component_max_count)
#define ECS_COMPONENT_TYPES(X)           \
    X(Test, TestComponent, kMaxEntities) \
    X(Test2, TestComponent, kMaxEntities)

// Create the component enum.
enum class EECSComponentType : u8 {
#define X(enum_name, ...) enum_name,
    ECS_COMPONENT_TYPES(X)
#undef X
    COUNT
};
static_assert((i32)EECSComponentType::COUNT < kMaxComponentTypes,
              "Too many component types defined!");
const char* ToString(EECSComponentType component_type);

template <typename T, i32 SIZE>
struct ECSComponentHolder {
    static constexpr i32 kMaxComponents = SIZE;

    std::array<ECSComponentIndex, kMaxEntities> EntityToComponent;
    std::array<ECSEntity, SIZE> ComponentToEntity;
    std::array<T, SIZE> Components = {};
    ECSComponentIndex NextComponent = 0;
    i32 ComponentCount = 0;

    void Init();
    void Shutdown() {}

    T* AddEntity(ECSEntity entity);
    void RemoveEntity(ECSEntity entity);
};

// COMPONENTS --------------------------------------------------------------------------------------

#define GENERATE_COMPONENT(component_name)                         \
    static constexpr const char* kComponentName = #component_name; \
    static constexpr EECSComponentType kComponentType = EECSComponentType::component_name;

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
void ECSComponentHolder<T, SIZE>::Init() {
    EECSComponentType component_type = T::kComponentType;

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
T* ECSComponentHolder<T, SIZE>::AddEntity(ECSEntity entity) {
    i32 entity_index = GetEntityIndex(entity);

    ASSERT(entity_index >= 0 && entity_index < kMaxEntities);
    ASSERT(ComponentCount < SIZE);
    ASSERT(EntityToComponent[entity_index] == NONE);

    // Find the next empty entity.
    ECSComponentIndex component_index = NextComponent;
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
void ECSComponentHolder<T, SIZE>::RemoveEntity(ECSEntity entity) {
    i32 entity_index = GetEntityIndex(entity);

    ASSERT(entity_index >= 0 && entity_index < kMaxEntities);
    ASSERT(ComponentCount > 0);

    // Get the component index for this entity
    ECSComponentIndex component_index = EntityToComponent[entity_index];
    ASSERT(component_index != NONE);

    // Update the translation arrays.
    EntityToComponent[entity_index] = NONE;
    ComponentToEntity[component_index] = NextComponent;
    NextComponent = component_index;

    ComponentCount--;
}

}  // namespace kdk

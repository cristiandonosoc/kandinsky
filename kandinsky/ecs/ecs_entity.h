#pragma once

#include <kandinsky/defines.h>
#include <kandinsky/math.h>

#include <SDL3/SDL.h>

#include <array>

namespace kdk {

using ECSEntity = i32;  // 8-bit generation, 24-bit index.
using ECSComponentIndex = i32;

static constexpr i32 kMaxEntities = 4096;
static constexpr i32 kMaxComponentTypes = 31;
static constexpr i32 kNewEntitySignature = 1 << kMaxComponentTypes;  // Just the first bit set.

// X macro for defining component types.
// Format: (component_enum_name, component_struct_name, component_max_count)
#define ECS_COMPONENT_TYPES(X) X(Transform, TransformComponent, kMaxEntities)

// Create the component enum.
enum class EECSComponentType : u8 {
#define X(enum_name, ...) enum_name,
    ECS_COMPONENT_TYPES(X)
#undef X
    COUNT
};
static_assert((i32)EECSComponentType::COUNT < kMaxComponentTypes);
const char* ToString(EECSComponentType component_type);

struct TransformComponent {
    static constexpr EECSComponentType ComponentType = EECSComponentType::Transform;
    Transform Transform = {};
};

using ECSEntitySignature = i32;
inline bool IsLive(const ECSEntitySignature& signature) { return signature < 0; }
bool Matches(const ECSEntitySignature& signature, EECSComponentType component_type);

inline i32 GetEntityIndex(ECSEntity entity) { return entity & 0xFFFFFF; }
inline u8 GetEntityGeneration(ECSEntity entity) { return (u8)(entity >> 24); }

template <typename T, i32 SIZE>
struct ECSComponentHolder {
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

struct ECSEntityManager {
    std::array<ECSEntitySignature, kMaxEntities> Signatures = {};
    std::array<u8, kMaxEntities> Generations = {};
    i32 NextIndex = 0;
    i32 EntityCount = 0;

    // Create the component arrays.
#define X(component_enum_name, component_struct_name, component_max_count, ...) \
    ECSComponentHolder<component_struct_name, component_max_count> component_enum_name##Holder;

    ECS_COMPONENT_TYPES(X)
#undef X
};

void Init(ECSEntityManager* eem);
void Shutdown(ECSEntityManager* eem);

ECSEntity CreateEntity(ECSEntityManager* eem);
void DestroyEntity(ECSEntityManager* eem, ECSEntity entity);

bool IsValid(const ECSEntityManager& eem, ECSEntity entity);
std::pair<bool, ECSEntitySignature*> GetEntitySignature(ECSEntityManager* eem, ECSEntity entity);

bool AddComponent(ECSEntityManager* eem, ECSEntity entity, EECSComponentType component_type);
bool RemoveComponent(ECSEntityManager* eem, ECSEntity entity, EECSComponentType component_type);

// TEMPLATE IMPLEMENTATION -------------------------------------------------------------------------

template <typename T, i32 SIZE>
void ECSComponentHolder<T, SIZE>::Init() {
    EECSComponentType component_type = T::ComponentType;

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
    ASSERT(entity >= 0 && entity < kMaxEntities);
    ASSERT(ComponentCount < SIZE);
    ASSERT(EntityToComponent[entity] == NONE);

    // Find the next empty entity.
    ECSComponentIndex index = NextComponent;
    ASSERT(index != NONE);

    // Update the translation arrays.
    NextComponent = ComponentToEntity[index];
    ComponentToEntity[index] = entity;
    EntityToComponent[entity] = index;

    ComponentCount++;

    // Reset the component.
    T* component = &Components[index];
    *component = {};

    return component;
}

template <typename T, i32 SIZE>
void ECSComponentHolder<T, SIZE>::RemoveEntity(ECSEntity entity) {
    ASSERT(entity >= 0 && entity < kMaxEntities);
    ASSERT(ComponentCount > 0);

    // Get the component index for this entity
    ECSComponentIndex index = EntityToComponent[entity];
    ASSERT(index != NONE);

    // Update the translation arrays.
    EntityToComponent[entity] = NONE;
    ComponentToEntity[index] = NextComponent;
    NextComponent = index;

    ComponentCount--;
}

}  // namespace kdk

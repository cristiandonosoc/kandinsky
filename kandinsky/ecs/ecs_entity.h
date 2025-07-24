#pragma once

#include <kandinsky/defines.h>

#include <array>

namespace kdk {

using ECSEntity = i32;
using ECSComponentIndex = i32;
using ECSEntitySignature = i32;

constexpr i32 kMaxEntities = 4096;

struct ECSEntityManager {
    std::array<ECSEntitySignature, kMaxEntities> Signatures = {};
    ECSEntity NextEntity = 0;
    i32 EntityCount = 0;
};

void Init(ECSEntityManager* eem);
void Shutdown(ECSEntityManager* eem);

ECSEntity CreateEntity(ECSEntityManager* eem);
void DestroyEntity(ECSEntityManager* eem, ECSEntity entity);

enum class EComponents : u8 {
    Transform = 0,
};

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

// TEMPLATE IMPLEMENTATION -------------------------------------------------------------------------

template <typename T, i32 SIZE>
void ECSComponentHolder<T, SIZE>::Init() {
    for (i32 i = 0; i < SIZE; i++) {
        Components[i] = i + 1;
    }
    Components.back() = NONE;

    for (auto& elem : EntityToComponent) {
        elem = NONE;
    }

    // Empty components point to the *next* empty component.
    // The last component points to NONE.
    for (i32 i = 0; i < SIZE; i++) {
        ComponentToEntity[i] = i + 1;
    }
    ComponentToEntity.back() = NONE;
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

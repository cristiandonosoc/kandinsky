#pragma once

#include <kandinsky/entity.h>
#include <kandinsky/graphics/light.h>

namespace kdk {

// TODO(cdc): Remove this eventually.
struct Box {
    GENERATE_ENTITY(Box);
};

struct EntityManager {
    static EntityManager* Get();
    static void Set(EntityManager* em);

    FixedArray<Box, GetMaxInstances(EEntityType::Box)> Boxes;
    FixedArray<DirectionalLight, GetMaxInstances(EEntityType::DirectionalLight)> DirectionalLights;
    FixedArray<PointLight, GetMaxInstances(EEntityType::PointLight)> PointLights;
    FixedArray<Spotlight, GetMaxInstances(EEntityType::Spotlight)> Spotlights;

    EntityID HoverEntityID = {};
    EntityID SelectedEntityID = {};
};

bool IsValid(const EntityManager& em);
void InitEntityManager(Arena* arena, EntityManager* em);

void* FindEntity(EntityManager* em, const EntityID& id);
template <typename T>
T* FindEntityT(EntityManager* em, const EntityID& id) {
    return (T*)FindEntity(em, id);
}

template <typename T>
T* AddEntityT(EntityManager* em, const Transform& transform = {}) {
    return (T*)AddEntity(em, T::StaticEntityType(), transform);
}
void* AddEntity(EntityManager* em, EEntityType type, const Transform& transform = {});

std::pair<void*, u32> GetTrack(EntityManager* em, EEntityType type);

template <typename T>
Iterator<T> GetIteratorT(EntityManager* em) {
    auto [ptr, size] = GetTrack(em, T::StaticEntityType());
    return {(T*)ptr, size, 0};
}

void UpdateModelMatrices(EntityManager* em);

}  // namespace kdk

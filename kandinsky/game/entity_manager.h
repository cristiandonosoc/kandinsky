#pragma once

#include <kandinsky/game/entity.h>
#include <kandinsky/game/spawner.h>
#include <kandinsky/game/tower.h>
#include <kandinsky/graphics/light.h>

namespace kdk {

struct SerdeArchive;

// TODO(cdc): Remove this eventually.
struct Box {
    GENERATE_ENTITY(Box);
};
void Serialize(SerdeArchive* sa, Box& box);
inline void BuildImGui(Box*) {}

struct EntityManager {
    static EntityManager* Get();
    static void Set(EntityManager* em);

#define X(enum_value, type_name, max_editor_instances, ...) \
    FixedArray<type_name, GetMaxInstances(EEntityType::enum_value)> type_name##s;

    ENTITY_TYPES(X)
#undef X

    EditorID HoverEntityID = {};
    EditorID SelectedEntityID = {};
};

bool IsValid(const EntityManager& em);
void InitEntityManager(Arena* arena, EntityManager* em);

void Serialize(SerdeArchive* sa, EntityManager& em);

void* FindEntity(EntityManager* em, EEntityType type, const EditorID& id);
inline void* FindEntity(EntityManager* em, const EditorID& id) {
    return FindEntity(em, id.GetEntityType(), id);
}


template <typename T>
T* FindEntityT(EntityManager* em, const EditorID& id) {
    return (T*)FindEntity(em, T::StaticEntityType(), id);
}

Entity* FindEntityOpaque(EntityManager* em, const EditorID& id);

template <typename T>
T* AddEntityT(EntityManager* em, const Transform& transform = {}) {
    return (T*)AddEntity(em, T::StaticEntityType(), transform);
}
void* AddEntity(EntityManager* em, EEntityType type, const Transform& transform = {});

bool DeleteEntity(EntityManager* em, const EditorID& id);

std::pair<void*, u32> GetTrack(EntityManager* em, EEntityType type);

template <typename T>
Iterator<T> GetIteratorT(EntityManager* em) {
    auto [ptr, size] = GetTrack(em, T::StaticEntityType());
    return {(T*)ptr, size, 0};
}

template <typename T>
Iterator<const T> GetIteratorT(const EntityManager* em) {
    auto [ptr, size] = GetTrack((EntityManager*)(em), T::StaticEntityType());
    return {(const T*)ptr, size, 0};
}

void UpdateModelMatrices(EntityManager* em);

}  // namespace kdk

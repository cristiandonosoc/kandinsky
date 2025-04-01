#include <kandinsky/game/entity_manager.h>

namespace kdk {

namespace entity_private {

EntityManager* g_EntityManager = nullptr;

template <typename Track>
void UpdateTrackModelMatrices(Track* track) {
    using namespace entity_private;
    for (u32 i = 0; i < track->Size; i++) {
        Entity& entity = (*track)[i].Entity;
        CalculateModelMatrix(entity.Transform, &entity.M_Model);
    }
}

template <typename T, u32 N>
void* AddEntityToTrack(FixedArray<T, N>* track, const Transform& transform) {
    using namespace entity_private;
    ASSERT(track->Size < track->Capacity());

    u32 index = track->Size;
    T& new_entity = track->Push({});
    new_entity.Entity.EditorID = GenerateNewEditorID(T::StaticEntityType());
    new_entity.Entity.InstanceID = {.Index = index,
                                    .Generation = 0,
                                    .EntityType = T::StaticEntityType()};
    new_entity.Entity.Transform = transform;
    return &new_entity;
}

}  // namespace entity_private

// EntityManager -----------------------------------------------------------------------------------

EntityManager* EntityManager::Get() { return entity_private::g_EntityManager; }
void EntityManager::Set(EntityManager* em) { entity_private::g_EntityManager = em; }

bool IsValid(const EntityManager&) {
    // for (u8 i = 1; i < (u8)EEntityType::COUNT; i++) {
    //     const EntityTrack& track = em.EntityTracks[i];
    //     if (!IsValid(track)) {
    //         return false;
    //     }
    // }

    return true;
}

void InitEntityManager(Arena*, EntityManager* em) {
    using namespace entity_private;

    // for (u8 i = 1; i < (u8)EEntityType::COUNT; i++) {
    //     EEntityType type = (EEntityType)i;

    //     // Using the switch to force a compile time update here.
    //     EntityTrack* track = &em->EntityTracks[i];

    //     std::pair<u32, u32> stats;
    //     SWITCH_ON_ENTITY_TRACK(type, GetTrackStats, stats);
    //     auto& [size, align] = stats;

    //     track->EntityType = type;
    //     track->MaxEntityCount = GetMaxInstances(type);
    //     track->Entities = ArenaPush(arena, size * track->MaxEntityCount, align);
    //     track->Transforms = ArenaPushArray<Transform>(arena, track->MaxEntityCount);
    //     track->ModelMatrices = ArenaPushArray<Mat4>(arena, track->MaxEntityCount);
    // }

    ASSERT(IsValid(*em));

    EntityManager::Set(em);
}

template <typename Track>
void* FindEntityByEditorID(Track* track, const EditorID& editor_id) {
    for (u32 i = 0; i < track->Size; i++) {
        if ((*track)[i].Entity.EditorID.Value == editor_id.Value) {
            return &(*track)[i];
        }
    }
    return nullptr;
}

void* FindEntity(EntityManager* em, EEntityType type, const EditorID& editor_id) {
    using namespace entity_private;
    switch (type) {
        case EEntityType::Invalid: ASSERT(false); break;
        case EEntityType::Box: return FindEntityByEditorID(&em->Boxes, editor_id);
        case EEntityType::DirectionalLight:
            return FindEntityByEditorID(&em->DirectionalLights, editor_id);
        case EEntityType::PointLight: return FindEntityByEditorID(&em->PointLights, editor_id);
        case EEntityType::Spotlight: return FindEntityByEditorID(&em->Spotlights, editor_id);
        case EEntityType::Tower: return FindEntityByEditorID(&em->Towers, editor_id);
        case EEntityType::COUNT: ASSERT(false); break;
    }

    ASSERT(false);
    return nullptr;
}

void* AddEntity(EntityManager* em, EEntityType type, const Transform& transform) {
    using namespace entity_private;

    switch (type) {
        case EEntityType::Invalid: ASSERT(false); return nullptr;
        case EEntityType::Box: return AddEntityToTrack(&em->Boxes, transform);
        case EEntityType::DirectionalLight:
            return AddEntityToTrack(&em->DirectionalLights, transform);
        case EEntityType::PointLight: return AddEntityToTrack(&em->PointLights, transform);
        case EEntityType::Spotlight: return AddEntityToTrack(&em->Spotlights, transform);
        case EEntityType::Tower: return AddEntityToTrack(&em->Towers, transform);
        case EEntityType::COUNT: ASSERT(false); return nullptr;
    }

    ASSERT(false);
    return nullptr;
}

std::pair<void*, u32> GetTrack(EntityManager* em, EEntityType type) {
    switch (type) {
        case EEntityType::Invalid: ASSERT(false); return {};
        case EEntityType::Box: return {em->Boxes.Data, em->Boxes.Size};
        case EEntityType::DirectionalLight:
            return {em->DirectionalLights.Data, em->DirectionalLights.Size};
        case EEntityType::PointLight: return {em->PointLights.Data, em->PointLights.Size};
        case EEntityType::Spotlight: return {em->Spotlights.Data, em->Spotlights.Size};
        case EEntityType::Tower: return {em->Towers.Data, em->Towers.Size};
        case EEntityType::COUNT: ASSERT(false); return {};
    }

    ASSERT(false);
    return {};
}

void UpdateModelMatrices(EntityManager* em) {
    using namespace entity_private;

    for (u32 i = 1; i < (u32)EEntityType::COUNT; i++) {
        EEntityType type = (EEntityType)i;
        switch (type) {
            case EEntityType::Invalid: ASSERT(false); return;
            case EEntityType::Box: UpdateTrackModelMatrices(&em->Boxes); continue;
            case EEntityType::DirectionalLight:
                UpdateTrackModelMatrices(&em->DirectionalLights);
                continue;
            case EEntityType::PointLight: UpdateTrackModelMatrices(&em->PointLights); continue;
            case EEntityType::Spotlight: UpdateTrackModelMatrices(&em->Spotlights); continue;
            case EEntityType::Tower: UpdateTrackModelMatrices(&em->Towers); continue;
            case EEntityType::COUNT: ASSERT(false); return;
        }

        ASSERT(false);
    }
}

}  // namespace kdk

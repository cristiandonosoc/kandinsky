#include <kandinsky/entity_manager.h>

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
    new_entity.Entity.EntityID = EntityID(T::StaticEntityType(), index);
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

void* FindEntity(EntityManager* em, const EntityID& id) {
    switch (id.GetEntityType()) {
        case EEntityType::Invalid: ASSERT(false); break;
        case EEntityType::Box: return &em->Boxes[id.GetIndex()];
        case EEntityType::Light: return &em->Lights[id.GetIndex()];
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
        case EEntityType::Light: return AddEntityToTrack(&em->Lights, transform);
        case EEntityType::COUNT: ASSERT(false); return nullptr;
    }

    ASSERT(false);
    return nullptr;
}

std::pair<void*, u32> GetTrack(EntityManager* em, EEntityType type) {
    switch (type) {
        case EEntityType::Invalid: ASSERT(false); return {};
        case EEntityType::Box: return {em->Boxes.Data, em->Boxes.Size};
        case EEntityType::Light: return {em->Lights.Data, em->Lights.Size};
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
            case EEntityType::Invalid: ASSERT(false); break;
            case EEntityType::Box: UpdateTrackModelMatrices(&em->Boxes); return;
            case EEntityType::Light: UpdateTrackModelMatrices(&em->Lights); return;
            case EEntityType::COUNT: ASSERT(false); break;
        }

        ASSERT(false);
    }
}

}  // namespace kdk

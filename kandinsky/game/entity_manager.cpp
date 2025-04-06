#include <kandinsky/game/entity_manager.h>

#include <kandinsky/serde.h>

namespace kdk {

void Serialize(SerdeArchive* sa, Box& box) { SERDE(sa, box, Entity); }

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

void Serialize(SerdeArchive* sa, EntityManager& em) {
#define X(enum_value, type_name, max_editor_instances, max_runtime_instances) \
    case EEntityType::enum_value: SERDE(sa, em, type_name##s); continue;

    for (u32 i = 1; i < (u32)EEntityType::COUNT; i++) {
        EEntityType type = (EEntityType)i;

        // clang-format off
    switch (type) {
        case EEntityType::Invalid: ASSERT(false); break;
        case EEntityType::COUNT: ASSERT(false);  break;
        ENTITY_TYPES(X);
    }
        // clang-format on

        ASSERT(false);
    }

#undef X
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

    // clang-format off
#define X(enum_value, type_name, max_editor_instances, max_runtime_instances) \
		case EEntityType::enum_value: return FindEntityByEditorID(&em->type_name##s, editor_id);

    switch (type) {
        case EEntityType::Invalid: ASSERT(false); break;
        case EEntityType::COUNT: ASSERT(false); break;
		ENTITY_TYPES(X)
    }

#undef X
    // clang-format on

    ASSERT(false);
    return nullptr;
}

void* AddEntity(EntityManager* em, EEntityType type, const Transform& transform) {
    using namespace entity_private;

    // clang-format off
#define X(enum_value, type_name, max_editor_instances, max_runtime_instances) \
    case EEntityType::enum_value: return AddEntityToTrack(&em->type_name##s, transform);

    switch (type) {
        case EEntityType::Invalid: ASSERT(false); break;
        case EEntityType::COUNT: ASSERT(false); break;
        ENTITY_TYPES(X)
    }
#undef X
    // clang-format on

    ASSERT(false);
    return nullptr;
}

std::pair<void*, u32> GetTrack(EntityManager* em, EEntityType type) {
    // clang-format off
#define X(enum_value, type_name, max_editor_instances, max_runtime_instances) \
    case EEntityType::enum_value: return {em->type_name##s.Data, em->type_name##s.Size};

    switch (type) {
        case EEntityType::Invalid: ASSERT(false); break;
        case EEntityType::COUNT: ASSERT(false); break;
        ENTITY_TYPES(X);
    }

#undef X
    // clang-format on

    ASSERT(false);
    return {};
}

void UpdateModelMatrices(EntityManager* em) {
    using namespace entity_private;

#define X(enum_value, type_name, max_editor_instances, max_runtime_instances) \
    case EEntityType::enum_value: UpdateTrackModelMatrices(&em->type_name##s); continue;

    for (u32 i = 1; i < (u32)EEntityType::COUNT; i++) {
        EEntityType type = (EEntityType)i;

        // clang-format off
    switch (type) {
        case EEntityType::Invalid: ASSERT(false); break;
        case EEntityType::COUNT: ASSERT(false);  break;
        ENTITY_TYPES(X);
    }
        // clang-format on

        ASSERT(false);
    }

#undef X
}

}  // namespace kdk

#include <kandinsky/entity.h>

#include <kandinsky/camera.h>
#include <kandinsky/graphics/light.h>
#include <kandinsky/graphics/opengl.h>
#include <kandinsky/imgui.h>
#include <kandinsky/memory.h>

namespace kdk {

const char* ToString(EEntityType entity_type) {
    switch (entity_type) {
        case EEntityType::Invalid: return "<INVALID>";
        case EEntityType::Box: return "Box";
        case EEntityType::Light: return "Light";
        case EEntityType::Camera: return "Camera";
        case EEntityType::COUNT: return "<COUNT>";
    }

    ASSERT(false);
    return "<UNKNOWN>";
}

u32 GetMaxInstances(EEntityType entity_type) {
    switch (entity_type) {
        case EEntityType::Invalid: ASSERT(false); return 0;
        case EEntityType::Box: return 64;
        case EEntityType::Light: return 16;
        case EEntityType::Camera: return 4;
        case EEntityType::COUNT: ASSERT(false); return 0;
    }

    ASSERT(false);
    return 0;
}

void BuildImgui(const EntityID& id) {
    ImGui::Text("%s - INDEX: %u (RAW: %u)", ToString(id.GetEntityType()), id.GetIndex(), id.ID);
}

// EntityTrack -------------------------------------------------------------------------------------

#define SWITCH_ON_ENTITY_TRACK(entity_type, fn, result, ...)               \
    switch (entity_type) {                                                 \
        case EEntityType::Invalid: ASSERT(false); break;                   \
        case EEntityType::Box: result = fn<Box>(__VA_ARGS__); break;       \
        case EEntityType::Light: result = fn<Light>(__VA_ARGS__); break;   \
        case EEntityType::Camera: result = fn<Camera>(__VA_ARGS__); break; \
        case EEntityType::COUNT: ASSERT(false); break;                     \
    }

namespace entity_private {

template <typename T>
std::pair<u32, u32> GetTrackStats() {
    return {(u32)sizeof(T), (u32)alignof(T)};
}

template <typename T>
void* AddEntity(EntityTrack* track, const Transform& transform) {
    ASSERT(track->EntityType == T::StaticEntityType());
    ASSERT(track->EntityCount < track->MaxEntityCount);

    u32 index = track->EntityCount++;

    T* entities = (T*)track->Entities;
    T& entity = entities[index];
    entity = {
        .ID = EntityID(track->EntityType, index),
    };

    Transform& et = track->Transforms[index];
    et = transform;
    et.Flags.Dirty = true;  // For now we're lazy.

    return &entity;
}

}  // namespace entity_private

bool IsValid(const EntityTrack& track) {
    if (track.MaxEntityCount == 0) {
        return false;
    }

    if (!track.Entities) {
        return false;
    }

    if (!track.Entities) {
        return false;
    }

    if (!track.ModelMatrices) {
        return false;
    }

    return true;
}

void* FindEntity(EntityTrack* track, const EntityID& id) {
    using namespace entity_private;

    ASSERT(track->EntityType == id.GetEntityType());
    u32 index = id.GetIndex();
    ASSERT(index < track->EntityCount);

    std::pair<u32, u32> stats;
    SWITCH_ON_ENTITY_TRACK(id.GetEntityType(), GetTrackStats, stats);
    auto& [size, align] = stats;

    return (void*)((u8*)track->Entities + size * index);
}

Transform& GetEntityTransform(EntityTrack* track, const EntityID& id) {
    u32 index = id.GetIndex();
    ASSERT(index < track->EntityCount);
    return track->Transforms[index];
}

Mat4* GetEntityModelMatrix(EntityTrack* track, const EntityID& id) {
    u32 index = id.GetIndex();
    ASSERT(index < track->EntityCount);
    return &track->ModelMatrices[index];
}

// EntityManager -----------------------------------------------------------------------------------

bool IsValid(const EntityManager& em) {
    for (u8 i = 1; i < (u8)EEntityType::COUNT; i++) {
        const EntityTrack& track = em.EntityTracks[i];
        if (!IsValid(track)) {
            return false;
        }
    }

    return true;
}

void InitEntityManager(Arena* arena, EntityManager* em) {
    using namespace entity_private;

    for (u8 i = 1; i < (u8)EEntityType::COUNT; i++) {
        EEntityType type = (EEntityType)i;

        // Using the switch to force a compile time update here.
        EntityTrack* track = &em->EntityTracks[i];

        std::pair<u32, u32> stats;
        SWITCH_ON_ENTITY_TRACK(type, GetTrackStats, stats);
        auto& [size, align] = stats;

        track->EntityType = type;
        track->MaxEntityCount = GetMaxInstances(type);
        track->Entities = ArenaPush(arena, size * track->MaxEntityCount, align);
        track->Transforms = ArenaPushArray<Transform>(arena, track->MaxEntityCount);
        track->ModelMatrices = ArenaPushArray<Mat4>(arena, track->MaxEntityCount);
    }

    ASSERT(IsValid(*em));
}

void* AddEntity(EntityManager* em, EEntityType type, const Transform& transform) {
    using namespace entity_private;

    auto* track = GetEntityTrack(em, type);
    ASSERT(track);

    void* result = nullptr;
    SWITCH_ON_ENTITY_TRACK(type, AddEntity, result, track, transform);

    ASSERT(result);
    return result;
}

void* FindEntity(EntityManager* em, const EntityID& id) {
    auto* track = GetEntityTrack(em, id.GetEntityType());
    ASSERT(track);
    return FindEntity(track, id);
}

std::pair<void*, EEntityType> GetSelectedEntity(EntityManager* em) {
    if (IsValid(em->SelectedEntityID)) {
        return {FindEntity(em, em->SelectedEntityID), em->SelectedEntityID.GetEntityType()};
    }

    return {nullptr, EEntityType::Invalid};
}

Transform& GetEntityTransform(EntityManager* em, const EntityID& id) {
    using namespace entity_private;

    auto* track = GetEntityTrack(em, id.GetEntityType());
    ASSERT(track);
    return GetEntityTransform(track, id);
}

const Mat4& GetEntityModelMatrix(EntityManager* em, const EntityID& id) {
    using namespace entity_private;

    auto* track = GetEntityTrack(em, id.GetEntityType());
    ASSERT(track);

    Mat4* mmodel = GetEntityModelMatrix(track, id);
    ASSERT(mmodel);

    Transform& transform = GetEntityTransform(em, id);
    if (transform.Flags.Dirty) {
        // We need to re-calculate the matrix.
        *mmodel = Mat4(1.0f);
        *mmodel = Translate(*mmodel, transform.GetPosition());
        *mmodel = Rotate(*mmodel, transform.GetRotation());
        *mmodel = Scale(*mmodel, Vec3(transform.GetScale()));
        transform.Flags.Dirty = false;
    }

    return *mmodel;
}

}  // namespace kdk

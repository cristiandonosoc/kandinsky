#include <kandinsky/entity.h>

#include <kandinsky/imgui.h>
#include <kandinsky/memory.h>

namespace kdk {

const char* ToString(EEntityType entity_type) {
    switch (entity_type) {
        case EEntityType::Invalid: return "<INVALID>";
        case EEntityType::Box: return "Box";
        case EEntityType::Model: return "Model";
        case EEntityType::Light: return "Light";
        case EEntityType::Camera: return "Camera";
        case EEntityType::COUNT: return "<COUNT>";
    }

    ASSERT(false);
    return "<UNKNOWN>";
}

void BuildImgui(const EntityID& id) {
    ImGui::Text("%s - INDEX: %u (RAW: %u)", ToString(id.GetEntityType()), id.GetIndex(), id.ID);
}

// EntityTrack -------------------------------------------------------------------------------------

namespace entity_private {

Entity* AddEntity(EntityTrack* track, const Transform& transform) {
    ASSERT(track->EntityCount < track->MaxEntityCount);
    u32 index = track->EntityCount++;

    Entity& entity = track->Entities[index];
    entity = Entity{
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

Entity* FindEntity(EntityTrack* track, const EntityID& id) {
    u32 index = id.GetIndex();
    ASSERT(index < track->EntityCount);
    return &track->Entities[index];
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
    for (u8 i = 1; i < (u8)EEntityType::COUNT; i++) {
        EEntityType type = (EEntityType)i;

        // Using the switch to force a compile time update here.
        EntityTrack* track = &em->EntityTracks[i];
        track->EntityType = type;
        switch (type) {
            case EEntityType::Invalid: ASSERT(false); continue;
            case EEntityType::Box: track->MaxEntityCount = 128; break;
            case EEntityType::Model: track->MaxEntityCount = 128; break;
            case EEntityType::Light: track->MaxEntityCount = 16; break;
            case EEntityType::Camera: track->MaxEntityCount = 4; break;
            case EEntityType::COUNT: break;
        }

        // Allocate the arrays to hold these maximums.
        track->Entities = ArenaPushArray<Entity>(arena, track->MaxEntityCount);
        track->Transforms = ArenaPushArray<Transform>(arena, track->MaxEntityCount);
        track->ModelMatrices = ArenaPushArray<Mat4>(arena, track->MaxEntityCount);
    }

    ASSERT(IsValid(*em));
}

Entity* AddEntity(EntityManager* em, EEntityType type, const Transform& transform) {
    using namespace entity_private;

    auto* track = GetEntityTrack(em, type);
    ASSERT(track);
    return AddEntity(track, transform);
}

Entity* FindEntity(EntityManager* em, const EntityID& id) {
    auto* track = GetEntityTrack(em, id.GetEntityType());
    ASSERT(track);
    return FindEntity(track, id);
}

Entity* GetSelectedEntity(EntityManager* em) {
    if (IsValid(em->SelectedEntityID)) {
        return FindEntity(em, em->SelectedEntityID);
    }

    return nullptr;
}

Transform& GetEntityTransform(EntityManager* em, const Entity& entity) {
    using namespace entity_private;

    auto* track = GetEntityTrack(em, entity.ID.GetEntityType());
    ASSERT(track);
    return GetEntityTransform(track, entity.ID);
}

const Mat4& GetEntityModelMatrix(EntityManager* em, const Entity& entity) {
    using namespace entity_private;

    auto* track = GetEntityTrack(em, entity.ID.GetEntityType());
    ASSERT(track);

    ASSERT(IsValid(entity));

    Mat4* mmodel = GetEntityModelMatrix(track, entity.ID);
    ASSERT(mmodel);

    Transform& transform = GetEntityTransform(em, entity);
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

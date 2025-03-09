#include <kandinsky/entity.h>

#include <kandinsky/imgui.h>

namespace kdk {

const char* ToString(EEntityType entity_type) {
    switch (entity_type) {
        case EEntityType::Invalid: return "<INVALID>";
        case EEntityType::Box: return "Box";
        case EEntityType::Model: return "Model";
        case EEntityType::Light: return "Light";
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

static Transform kEmptyTransform = {};

Entity* AddEntity(EntityTrack* track, const Transform& transform) {
    ASSERT(track->EntityCount < EntityTrack::kMaxEntityCount);
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

Entity* AddEntity(EntityManager* em, EEntityType type, const Transform& transform) {
    using namespace entity_private;

    switch (type) {
        case EEntityType::Invalid: ASSERT(false); return nullptr;
        case EEntityType::Box: return AddEntity(&em->Boxes, transform);
        case EEntityType::Model: return AddEntity(&em->Models, transform);
        case EEntityType::Light: return AddEntity(&em->Lights, transform);
        case EEntityType::COUNT: ASSERT(false); return nullptr;
    }

    ASSERT(false);
    return nullptr;
}

Entity* FindEntity(EntityManager* em, const EntityID& id) {
    using namespace entity_private;

    ASSERT(IsValid(id));
    EEntityType entity_type = id.GetEntityType();
    switch (entity_type) {
        case EEntityType::Invalid: ASSERT(false); return nullptr;
        case EEntityType::Box: return FindEntity(&em->Boxes, id);
        case EEntityType::Model: return FindEntity(&em->Models, id);
        case EEntityType::Light: return FindEntity(&em->Lights, id);
        case EEntityType::COUNT: ASSERT(false); return nullptr;
    }

    ASSERT(false);
    return nullptr;
}

Entity* GetSelectedEntity(EntityManager* em) {
    if (IsValid(em->SelectedEntityID)) {
        return FindEntity(em, em->SelectedEntityID);
    }

    return nullptr;
}

Transform& GetEntityTransform(EntityManager* em, const Entity& entity) {
    using namespace entity_private;

    ASSERT(IsValid(entity));
    EEntityType entity_type = entity.ID.GetEntityType();
    switch (entity_type) {
        case EEntityType::Invalid: ASSERT(false); return kEmptyTransform;
        case EEntityType::Box: return GetEntityTransform(&em->Boxes, entity.ID);
        case EEntityType::Model: return GetEntityTransform(&em->Models, entity.ID);
        case EEntityType::Light: return GetEntityTransform(&em->Lights, entity.ID);
        case EEntityType::COUNT: ASSERT(false); return kEmptyTransform;
    }

    ASSERT(false);
    return kEmptyTransform;
}

const Mat4& GetEntityModelMatrix(EntityManager* em, const Entity& entity) {
    using namespace entity_private;

    ASSERT(IsValid(entity));

    Mat4* mmodel = nullptr;
    EEntityType entity_type = entity.ID.GetEntityType();
    switch (entity_type) {
        case EEntityType::Invalid: ASSERT(false); break;
        case EEntityType::Box: mmodel = GetEntityModelMatrix(&em->Boxes, entity.ID); break;
        case EEntityType::Model: mmodel = GetEntityModelMatrix(&em->Models, entity.ID); break;
        case EEntityType::Light: mmodel = GetEntityModelMatrix(&em->Lights, entity.ID); break;
        case EEntityType::COUNT: ASSERT(false); break;
    }

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

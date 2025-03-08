#include <kandinsky/entity.h>

namespace kdk {

Entity* AddEntity(EntityManager* em, EEntityType type, const Transform& transform) {
    ASSERT(em->EntityCount < EntityManager::kMaxEntityCount);
    u32 id = em->EntityCount++;

    Entity& entity = em->Entities[id];
    entity = Entity{
        .ID = id,
        .Type = type,
    };

    Transform& et = em->Transforms[id];
    et = transform;
    et.Flags.Dirty = true;  // For now we're lazy.

    return &entity;
}

Entity* GetSelectedEntity(EntityManager& em) {
    if (em.SelectedEntityID != 0) {
        return &em.Entities[em.SelectedEntityID];
    }

    return nullptr;
}

Transform& GetEntityTransform(EntityManager* em, const Entity& entity) {
    ASSERT(IsValid(entity));
    return em->Transforms[entity.ID];
}

const Mat4& GetEntityModelMatrix(EntityManager* em, const Entity& entity) {
    ASSERT(IsValid(entity));

    Transform& transform = GetEntityTransform(em, entity);
    if (transform.Flags.Dirty) {
        // We need to calculate the matrix.
        Mat4 mmodel = Mat4(1.0f);
        mmodel = Translate(mmodel, transform.GetPosition());
        mmodel = Rotate(mmodel, transform.GetRotation());
        mmodel = Scale(mmodel, Vec3(transform.GetScale()));

        em->ModelMatrices[entity.ID] = mmodel;
        transform.Flags.Dirty = false;
    }

    return em->ModelMatrices[entity.ID];
}

}  // namespace kdk

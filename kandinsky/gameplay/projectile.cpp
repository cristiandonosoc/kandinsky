#include <kandinsky/gameplay/projectile.h>

#include <kandinsky/platform.h>

namespace kdk {

void Update(Entity* entity, ProjectileEntity* projectile, float dt) {
    (void)entity;
    (void)projectile;
    (void)dt;

    // Move towards target if valid.
    auto* ps = platform::GetPlatformContext();
    if (IsValid(*ps->EntityManager, projectile->Target)) {
        Entity* target_entity = GetEntity(ps->EntityManager, projectile->Target);
        ASSERT(target_entity);

        Vec3 separation = target_entity->Transform.Position - entity->Transform.Position;
        Vec3 dir = Normalize(separation);

        entity->Transform.Position += dir * projectile->MoveSpeed * dt;

        constexpr float kMinDistance = 0.25f * 0.25f;

        // See if we've reached the target.
        if (LengthSq(separation) < kMinDistance) {
            SDL_Log("Hit!");
            // Close enough.
            DestroyEntity(ps->EntityManager, entity->ID);
            return;
        }

        entity->Transform.Position = target_entity->Transform.Position;
    }
}

std::pair<EntityID, Entity*> CreateProjectile(EntityManager* em,
                                              EProjectileType projectile_type,
                                              const CreateEntityOptions& options,
                                              EntityID target) {
    ProjectileEntity initial_values = {
        .Type = projectile_type,
        .Target = target,
    };
    auto [id, entity] = CreateEntity<ProjectileEntity>(em, options, &initial_values);
    ASSERT(IsValid(*em, id));

    return {id, entity};
}

}  // namespace kdk

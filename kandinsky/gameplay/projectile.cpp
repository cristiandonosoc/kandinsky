#include <kandinsky/gameplay/projectile.h>

#include <kandinsky/platform.h>

namespace kdk {

namespace projectile_private {

bool MoveTowardsTarget(ProjectileEntity* projectile, float dt) {
    Vec3 separation = projectile->LastTargetPosition - projectile->GetTransform().Position;
    Vec3 dir = Normalize(separation);

    float move_delta = projectile->MoveSpeed * dt;
    projectile->GetTransform().Position += dir * move_delta;

    constexpr float kMinDistance = 0.25f * 0.25f;

    // See if we've reached the target.
    if (LengthSq(separation) < kMinDistance) {
        return true;
    }

    return false;
}

}  // namespace projectile_private

void Update(ProjectileEntity* projectile, float dt) {
    using namespace projectile_private;

    // Move towards target if valid.
    auto* ps = platform::GetPlatformContext();
    if (IsValid(*ps->EntityManager, projectile->Target)) {
        Entity* target_entity = GetEntity(ps->EntityManager, projectile->Target);
        ASSERT(target_entity);

        projectile->LastTargetPosition = target_entity->Transform.Position;

        if (MoveTowardsTarget(projectile, dt)) {
            if (auto [_, health] =
                    GetComponent<HealthComponent>(ps->EntityManager, projectile->Target);
                health) {
                ReceiveDamage(health, projectile->Damage);
            }

            SDL_Log("Hit!!");
            DestroyEntity(ps->EntityManager, projectile->GetEntityID());
            return;
        }
    } else {
        if (MoveTowardsTarget(projectile, dt)) {
            SDL_Log("Hit!!");
            DestroyEntity(ps->EntityManager, projectile->GetEntityID());
            return;
        }
    }
}

std::pair<EntityID, Entity*> CreateProjectile(EntityManager* em,
                                              EProjectileType projectile_type,
                                              const CreateEntityOptions& options,
                                              EntityID target) {
    ProjectileEntity initial_values = {
        .Target = target,
        .Type = projectile_type,
    };
    auto [id, entity] = CreateEntity<ProjectileEntity>(em, options, &initial_values);
    ASSERT(IsValid(*em, id));

    return {id, entity};
}

}  // namespace kdk

#include <kandinsky/gameplay/building.h>

#include <kandinsky/gameplay/health_component.h>
#include <kandinsky/gameplay/projectile.h>
#include <kandinsky/platform.h>

namespace kdk {

void Update(Entity* entity, BuildingEntity* building, float dt) {
    (void)entity;
    (void)building;
    (void)dt;

    auto* ps = platform::GetPlatformContext();

    float target_time = building->LastShot + building->ShootInterval;
    float now = (float)ps->CurrentTimeTracking->TotalSeconds;
    if (now < target_time) {
        return;
    }
    building->LastShot = now;
    SDL_Log("Shoot!");

    // Get an enemy.
    EntityID enemy_id = {};
    VisitEntities<EnemyEntity>(ps->EntityManager, [&enemy_id](EntityID id, Entity*, EnemyEntity*) {
        enemy_id = id;
        return false;  // Stop after the first one.
    });

    CreateEntityOptions options = {
        .Transform = entity->Transform,
    };
    CreateProjectile(ps->EntityManager, EProjectileType::Base, options, enemy_id);
}

std::pair<EntityID, Entity*> CreateBuilding(EntityManager* em,
                                            EBuildingType building_type,
                                            const CreateEntityOptions& options) {
    auto [id, entity] = CreateEntity(em, EEntityType::Building, options);
    ASSERT(IsValid(*em, id));

    switch (building_type) {
        case EBuildingType::Base: {
            AddComponent<HealthComponent>(em, id);
            break;
        }
        case EBuildingType::Invalid: ASSERT(false); break;
        case EBuildingType::COUNT: ASSERT(false); break;
    }

    return {id, entity};
}

}  // namespace kdk

#include <kandinsky/gameplay/building.h>

#include <kandinsky/debug.h>
#include <kandinsky/gameplay/health_component.h>
#include <kandinsky/gameplay/projectile.h>
#include <kandinsky/platform.h>
#include <limits>

namespace kdk {

namespace building_private {

EntityID GetClosestEntity(PlatformState* ps, Entity* building_entity) {
    // Get an enemy.
    EntityID enemy_id = {};
    float current_dist_sq = std::numeric_limits<float>::max();
    VisitEntities<EnemyEntity>(ps->EntityManager,
                               [building_entity, &enemy_id, &current_dist_sq](EntityID id,
                                                                              Entity* enemy_entity,
                                                                              EnemyEntity*) {
                                   float dist_sq = LengthSq(enemy_entity->Transform.Position -
                                                            building_entity->Transform.Position);
                                   if (dist_sq < current_dist_sq) {
                                       current_dist_sq = dist_sq;
                                       enemy_id = id;
                                   }
                                   return true;
                               });
    return enemy_id;
}

}  // namespace building_private

void Update(Entity* entity, BuildingEntity* building, float dt) {
    (void)entity;
    (void)dt;

    auto* ps = platform::GetPlatformContext();

    float target_time = building->LastShot + building->ShootInterval;
    float now = (float)ps->CurrentTimeTracking->TotalSeconds;
    if (now < target_time) {
        return;
    }
    building->LastShot = now;

    Shoot(building);
}

void BuildImGui(BuildingEntity* building) {
    if (ImGui::Button("Shoot")) {
        Shoot(building);
    }
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

void Shoot(BuildingEntity* building) {
    using namespace building_private;

    auto* ps = platform::GetPlatformContext();

    Entity* entity = building->GetEntity();

    EntityID enemy_id = GetClosestEntity(ps, entity);
    if (!IsValid(*ps->EntityManager, enemy_id)) {
        return;
    }

    CreateEntityOptions options = {
        .Transform = entity->Transform,
    };
    CreateProjectile(ps->EntityManager, EProjectileType::Base, options, enemy_id);

    Vec3 from = entity->Transform.Position;
    Vec3 to = GetEntity(ps->EntityManager, enemy_id)->Transform.Position;

    Schedule(&ps->ScheduleSystem, "ShootDebug"sv, 1, [from, to](PlatformState* ps) {
        std::pair<Vec3, Vec3> line{from, to};
        Debug::DrawLines(ps, MakeSpan(line), Color32::Red, 2);
    });

    SDL_Log("Shoot!");
}

}  // namespace kdk

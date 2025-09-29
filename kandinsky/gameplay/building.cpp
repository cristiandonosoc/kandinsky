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

void UpdateTower(PlatformState* ps, BuildingEntity* building, float dt) {
    (void)dt;

    float target_time = building->LastShot + building->ShootInterval;
    float now = (float)ps->CurrentTimeTracking->TotalSeconds;
    if (now < target_time) {
        return;
    }
    building->LastShot = now;

    Shoot(building);
}

void UpdateBase(PlatformState* ps, BuildingEntity* building, float dt) {
    (void)ps;
    (void)building;
    (void)dt;
}

}  // namespace building_private

void Update(BuildingEntity* building, float dt) {
    using namespace building_private;
    auto* ps = platform::GetPlatformContext();

    // TODO(cdc): Remove this hack soon.
    if (building->Type == EBuildingType::Invalid) {
        building->Type = EBuildingType::Tower;
        return;
    }

    switch (building->Type) {
        case EBuildingType::Tower: {
            UpdateTower(ps, building, dt);
            break;
        }
        case EBuildingType::Base: {
            UpdateBase(ps, building, dt);
            break;
        }
        case EBuildingType::Invalid: ASSERT(false); break;
        case EBuildingType::COUNT: ASSERT(false); break;
    }
}

void BuildImGui(BuildingEntity* building) {
    if (ImGui::Button("Shoot")) {
        Shoot(building);
    }
}

std::pair<EntityID, BuildingEntity*> CreateBuilding(EntityManager* em,
                                                    EBuildingType building_type,
                                                    const CreateEntityOptions& options) {
    auto [id, building] = CreateEntity<BuildingEntity>(em, options);
    ASSERT(building);
    building->Type = building_type;

    switch (building_type) {
        case EBuildingType::Tower: {
            AddComponent<HealthComponent>(em, id);
            break;
        }
        case EBuildingType::Base: {
            AddComponent<HealthComponent>(em, id);
            break;
        }
        case EBuildingType::Invalid: ASSERT(false); break;
        case EBuildingType::COUNT: ASSERT(false); break;
    }

    return {id, building};
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

    auto* ss = GetSystem<ScheduleSystem>(&ps->Systems);

    Schedule(ss, "ShootDebug"sv, 1, [from, to](PlatformState* ps) {
        std::pair<Vec3, Vec3> line{from, to};
        Debug::DrawLines(ps, MakeSpan(line), Color32::Red, 2);
    });
}

}  // namespace kdk

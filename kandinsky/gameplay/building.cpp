#include <imgui.h>
#include <kandinsky/gameplay/building.h>

#include <kandinsky/debug.h>
#include <kandinsky/gameplay/gamemode.h>
#include <kandinsky/gameplay/health_component.h>
#include <kandinsky/gameplay/projectile.h>
#include <kandinsky/imgui_widgets.h>
#include <kandinsky/platform.h>
#include "kandinsky/core/defines.h"

namespace kdk {

String ToString(EBuildingType building_type) {
    switch (building_type) {
        case EBuildingType::Invalid: return "<invalid>"sv;
        case EBuildingType::Tower: return "Tower"sv;
        case EBuildingType::Base: return "Base"sv;
        case EBuildingType::COUNT: ASSERT(false); return "<count>"sv;
    }

    ASSERT(false);
    return "<unknown>"sv;
}

namespace building_private {

EntityID GetClosestEntity(PlatformState* ps, BuildingEntity* building) {
    // Get an enemy.
    EntityID enemy_id = {};
    float current_dist_sq = std::numeric_limits<float>::max();
    VisitEntities<EnemyEntity>(
        ps->EntityManager,
        [building, &enemy_id, &current_dist_sq](EntityID id, EnemyEntity* enemy) {
            float dist_sq =
                LengthSq(enemy->GetTransform().Position - building->GetTransform().Position);
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

void Serialize(SerdeArchive* sa, BuildingEntity* building) {
    SERDE(sa, building, BuildingType);

    // TODO(cdc): Do per type serialization?

    // Tower.
    SERDE(sa, building, ShootInterval);

    // Base.
    SERDE(sa, building, Lives);
}

void Update(BuildingEntity* building, float dt) {
    using namespace building_private;
    auto* ps = platform::GetPlatformContext();

    // TODO(cdc): Remove this hack soon.
    if (building->BuildingType == EBuildingType::Invalid) {
        building->BuildingType = EBuildingType::Tower;
        return;
    }

    switch (building->BuildingType) {
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
    building->BuildingType = ImGui_EnumCombo("Building Type"sv, building->BuildingType);
    // IMGUI_DISABLED_SCOPE() { ImGui::InputInt2("Grid Coord", &building->GridCoord.x); }

    ImGui::InputFloat("Shoot Interval", &building->ShootInterval);
    ImGui::Text("Last Shot: %.2f", building->LastShot);
    ImGui::BeginDisabled();
    ImGui::DragFloat("Lives", &building->Lives, 1.0f, 0.0f, 100.0f);
    ImGui::EndDisabled();

    if (ImGui::Button("Shoot")) {
        Shoot(building);
    }
}

std::pair<EntityID, BuildingEntity*> CreateBuilding(EntityManager* em,
                                                    EBuildingType building_type,
                                                    const CreateEntityOptions& options) {
    auto [id, building] = CreateEntity<BuildingEntity>(em, options);
    ASSERT(building);
    building->BuildingType = building_type;

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

    EntityID enemy_id = GetClosestEntity(ps, building);
    if (!IsValid(*ps->EntityManager, enemy_id)) {
        return;
    }

    CreateEntityOptions options = {
        .Transform = building->GetTransform(),
    };
    CreateProjectile(ps->EntityManager, EProjectileType::Base, options, enemy_id);

    Vec3 from = building->GetTransform().Position;
    Vec3 to = GetEntity(ps->EntityManager, enemy_id)->Transform.Position;

    auto* ss = GetSystem<ScheduleSystem>(&ps->Systems);
    Schedule(ss, "ShootDebug"sv, 1, [from, to](PlatformState* ps) {
        std::pair<Vec3, Vec3> line{from, to};
        Debug::DrawLines(ps, MakeSpan(line), Color32::Red, 2);
    });
}

void Hit(BuildingEntity* building, EnemyEntity* enemy) {
    (void)enemy;
    if (building->BuildingType == EBuildingType::Base) {
        building->Lives -= 1.0f;
        if (building->Lives <= 0.0f) {
            auto* ps = platform::GetPlatformContext();
            BuildingDestroyed(&ps->GameMode, building);
        }
    }
}

}  // namespace kdk

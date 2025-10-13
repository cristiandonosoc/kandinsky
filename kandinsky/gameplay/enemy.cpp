#include <kandinsky/gameplay/enemy.h>

#include <kandinsky/gameplay/health_component.h>
#include <kandinsky/platform.h>
#include "kandinsky/entity.h"

namespace kdk {

void Update(EnemyEntity* enemy, float dt) {
    auto* ps = platform::GetPlatformContext();

    if (!IsValid(*ps->EntityManager, enemy->TargetBase)) {
        // enemy->GetTransform().Position.x += enemy->MoveSpeed * dt;
        return;
    }

    const Vec3& pos = enemy->GetTransform().Position;

    auto* base = GetTypedEntity<BuildingEntity>(ps->EntityManager, enemy->TargetBase);
    ASSERT(base);

    constexpr float kMinDistance = SQUARE(1);
    if (DistanceSq(base->GetEntity()->Transform.Position, pos) < kMinDistance) {
        Hit(base, enemy);
        DestroyEntity(ps->EntityManager, enemy->GetEntityID());
        return;
    }

    IVec2 grid_coord = GetGridCoord(*enemy->GetEntity());

    auto target_tile = GetFlowTileSafe(ps->CurrentScene->Terrain, grid_coord.x, grid_coord.y);
    if (!target_tile.HasValue()) {
        return;
    }

    Vec3 target_pos = Vec3((float)target_tile->x, pos.y, (float)target_tile->y);
    Vec3 dir = target_pos - pos;

    Vec3 ndir = Normalize(dir);

    enemy->GetTransform().Position += ndir * enemy->MoveSpeed * dt;

    // float distance = Length(dir);
    // if (distance < 1.0f) {
    // 	auto [_, health] = GetComponent<HealthComponent>(ps->EntityManager, base->GetEntityID());
    // 	if (health) {
    // 		health->CurrentHealth -= 10.0f * dt;
    // 		if (health->CurrentHealth <= 0.0f) {
    // 			SDL_Log("Entity %s: Destroyed base!", enemy->GetName().Str());
    // 			DestroyEntity(ps->EntityManager, base->GetEntityID());
    // 		}
    // 	}
    // }
}

}  // namespace kdk

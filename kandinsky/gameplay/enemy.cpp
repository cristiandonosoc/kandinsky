#include <kandinsky/gameplay/enemy.h>

#include <kandinsky/gameplay/health_component.h>
#include <kandinsky/platform.h>

namespace kdk {

void Update(EnemyEntity* enemy, float dt) {
    auto* ps = platform::GetPlatformContext();

    if (!IsValid(*ps->EntityManager, enemy->TargetBase)) {
        enemy->GetTransform().Position.x += enemy->MoveSpeed * dt;
        return;
    }

    auto* base = GetTypedEntity<BuildingEntity>(ps->EntityManager, enemy->TargetBase);
    ASSERT(base);

    Vec3 dir = base->GetTransform().Position - enemy->GetTransform().Position;
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

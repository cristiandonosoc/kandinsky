#include <kandinsky/systems/enemy_system.h>

#include <kandinsky/gameplay/building.h>
#include <kandinsky/gameplay/gamemode.h>
#include <kandinsky/gameplay/health_component.h>

namespace kdk {

std::pair<EntityID, EnemyEntity*> CreateEnemy(EnemySystem* es,
                                              EEnemyType enemy_type,
                                              const CreateEntityOptions& options) {
    auto [id, enemy] = CreateEntity<EnemyEntity>(es->GetEntityManager(), options);
    ASSERT(enemy);

    switch (enemy_type) {
        case EEnemyType::Base: {
            AddComponent<HealthComponent>(es->GetEntityManager(), id);
            enemy->TargetBase = es->GetGameMode()->Base;
            break;
        }
        case EEnemyType::Invalid: ASSERT(false); break;
        case EEnemyType::COUNT: ASSERT(false); break;
    }

    return {id, enemy};
}

}  // namespace kdk

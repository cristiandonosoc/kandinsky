#include <kandinsky/gameplay/enemy.h>

#include <kandinsky/gameplay/health_component.h>

namespace kdk {

void Update(EnemyEntity* enemy, float dt) {
    (void)enemy;
    (void)dt;

    enemy->GetTransform().Position.x += enemy->MoveSpeed * dt;
}

std::pair<EntityID, EnemyEntity*> CreateEnemy(EntityManager* em,
                                              EEnemyType enemy_type,
                                              const CreateEntityOptions& options) {
    auto [id, enemy] = CreateEntity<EnemyEntity>(em, options);
    ASSERT(IsValid(*em, id));

    switch (enemy_type) {
        case EEnemyType::Base: {
            AddComponent<HealthComponent>(em, id);
            break;
        }
        case EEnemyType::Invalid: ASSERT(false); break;
        case EEnemyType::COUNT: ASSERT(false); break;
    }

    return {id, enemy};
}

}  // namespace kdk

#include <kandinsky/gameplay/enemy.h>

#include <kandinsky/gameplay/health_component.h>

namespace kdk {

void Update(Entity* entity, EnemyEntity* enemy, float dt) {
    (void)entity;
    (void)enemy;
    (void)dt;

    entity->Transform.Position.x += enemy->MoveSpeed * dt;
}

std::pair<EntityID, Entity*> CreateEnemy(EntityManager* em,
                                         EEnemyType enemy_type,
                                         const CreateEntityOptions& options) {
    auto [id, entity] = CreateEntity(em, EEntityType::Enemy, options);
    ASSERT(IsValid(*em, id));

    switch (enemy_type) {
        case EEnemyType::Base: {
            AddComponent<HealthComponent>(em, id);
            break;
        }
        case EEnemyType::Invalid: ASSERT(false); break;
        case EEnemyType::COUNT: ASSERT(false); break;
    }

    return {id, entity};
}

}  // namespace kdk

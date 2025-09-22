#include <kandinsky/gameplay/enemy.h>

namespace kdk {

void Update(Entity* entity, EnemyEntity* enemy, float dt) {
    (void)entity;
    (void)enemy;
    (void)dt;

    entity->Transform.Position.x += enemy->MoveSpeed * dt;
}

}  // namespace kdk

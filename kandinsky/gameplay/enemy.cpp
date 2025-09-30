#include <kandinsky/gameplay/enemy.h>

#include <kandinsky/gameplay/health_component.h>

namespace kdk {

void Update(EnemyEntity* enemy, float dt) {
    (void)enemy;
    (void)dt;

    enemy->GetTransform().Position.x += enemy->MoveSpeed * dt;
}

}  // namespace kdk

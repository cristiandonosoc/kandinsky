#include <kandinsky/gameplay/player.h>

namespace kdk {

void Update(Entity* entity, PlayerEntity* player, float dt) {
    (void)player;
    AddRotation(&entity->Transform, Vec3(1.0f, 0.0f, 0.0f), (float)(25.0f * dt));
}

}  // namespace kdk

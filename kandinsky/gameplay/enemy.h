#pragma once

#include <kandinsky/entity.h>

namespace kdk {

struct EnemyEntity {
	GENERATE_ENTITY(Enemy);
};

void Update(Entity* entity, EnemyEntity* enemy, float dt);

} // namespace kdk

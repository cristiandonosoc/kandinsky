#pragma once

#include <kandinsky/entity.h>

namespace kdk {

struct EnemyEntity {
	GENERATE_ENTITY(Enemy);

	float MoveSpeed = 1.0f;
};

void Update(Entity* entity, EnemyEntity* enemy, float dt);
inline void Serialize(SerdeArchive*, EnemyEntity*) {}

} // namespace kdk

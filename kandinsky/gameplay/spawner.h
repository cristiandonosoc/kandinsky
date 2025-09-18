#pragma once

#include <kandinsky/entity.h>

namespace kdk {

struct SpawnerEntity {
	GENERATE_ENTITY(Spawner);

	// Time until next spawn.
	float SpawnInterval = 2.0f;
	float LastSpawnTime = 0.0f;
};

void Update(Entity* entity, SpawnerEntity* spawner, float dt);

} // namespace kdk

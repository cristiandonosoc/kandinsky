#pragma once

#include <kandinsky/entity.h>

namespace kdk {

struct SpawnerEntity {
    GENERATE_ENTITY(Spawner);

    // Time until next spawn.
    float SpawnInterval = 2.0f;
    float LastSpawnTime = 0.0f;
};
void Validate(const Scene*, const SpawnerEntity& spawner, FixedVector<ValidationError, 64>*);

void Update(SpawnerEntity* spawner, float dt);
void Serialize(SerdeArchive* sa, SpawnerEntity* spawner);
void BuildImGui(SpawnerEntity* spawner);

}  // namespace kdk

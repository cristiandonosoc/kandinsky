#pragma once

#include <kandinsky/entity.h>
#include <kandinsky/gameplay/enemy.h>
#include <kandinsky/systems/system.h>

namespace kdk {

struct EnemySystem {
    GENERATE_SYSTEM(Enemy);

    EntityID TargetBase = {};
};

std::pair<EntityID, EnemyEntity*> CreateEnemy(EnemySystem* es,
                                              EEnemyType enemy_type,
                                              const CreateEntityOptions& options);

}  // namespace kdk

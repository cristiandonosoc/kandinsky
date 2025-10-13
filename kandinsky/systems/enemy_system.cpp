#include <kandinsky/systems/enemy_system.h>

#include <kandinsky/gameplay/health_component.h>
#include "kandinsky/gameplay/building.h"

namespace kdk {

void Start(EnemySystem* es) {
    es->TargetBase = {};

    // Search for the base.
    VisitEntities<BuildingEntity>(es->GetEntityManager(),
                                  [es](EntityID id, BuildingEntity* building) {
                                      if (building->BuildingType == EBuildingType::Base) {
                                          es->TargetBase = id;
                                          return false;
                                      }

                                      return true;
                                  });
}

void Stop(EnemySystem* es) { es->TargetBase = {}; }

std::pair<EntityID, EnemyEntity*> CreateEnemy(EnemySystem* es,
                                              EEnemyType enemy_type,
                                              const CreateEntityOptions& options) {
    auto [id, enemy] = CreateEntity<EnemyEntity>(es->GetEntityManager(), options);
    ASSERT(enemy);

    switch (enemy_type) {
        case EEnemyType::Base: {
            AddComponent<HealthComponent>(es->GetEntityManager(), id);
            enemy->TargetBase = es->TargetBase;
            break;
        }
        case EEnemyType::Invalid: ASSERT(false); break;
        case EEnemyType::COUNT: ASSERT(false); break;
    }

    return {id, enemy};
}

}  // namespace kdk

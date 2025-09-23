#pragma once

#include <kandinsky/entity.h>

namespace kdk {

enum class EEnemyType : u8 {
    Invalid = 0,
    Base,
    COUNT,
};

struct EnemyEntity {
    GENERATE_ENTITY(Enemy);

    EEnemyType Type = EEnemyType::Invalid;
    float MoveSpeed = 1.0f;
};

void Update(Entity* entity, EnemyEntity* enemy, float dt);
inline void Serialize(SerdeArchive*, EnemyEntity*) {}

std::pair<EntityID, Entity*> CreateEnemy(EntityManager* em,
                                         EEnemyType enemy_type,
                                         const CreateEntityOptions& options);

}  // namespace kdk

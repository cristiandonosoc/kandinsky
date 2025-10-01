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
    EntityID TargetBase = {};
    float MoveSpeed = 1.0f;
};

void Update(EnemyEntity* enemy, float dt);
inline void Serialize(SerdeArchive*, EnemyEntity*) {}

}  // namespace kdk

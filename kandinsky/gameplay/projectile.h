#pragma once

#include <kandinsky/entity.h>

namespace kdk {

enum class EProjectileType : u8 {
    Invalid = 0,
    Base,
    COUNT,
};

struct ProjectileEntity {
    GENERATE_ENTITY(Projectile);

    EProjectileType Type = EProjectileType::Invalid;
    float MoveSpeed = 2.0f;
    EntityID Target = {};
};

void Update(Entity* entity, ProjectileEntity* projectile, float dt);
inline void Serialize(SerdeArchive*, ProjectileEntity*) {}

std::pair<EntityID, Entity*> CreateProjectile(EntityManager* em,
                                              EProjectileType projectile_type,
                                              const CreateEntityOptions& options,
                                              EntityID target);

}  // namespace kdk

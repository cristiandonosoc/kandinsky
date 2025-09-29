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

    Vec3 LastTargetPosition = {};
    float MoveSpeed = 5.0f;
    float Damage = 25.0f;
    EntityID Target = {};
    EProjectileType Type = EProjectileType::Invalid;
};

void Update(ProjectileEntity* projectile, float dt);
inline void Serialize(SerdeArchive*, ProjectileEntity*) {}

std::pair<EntityID, Entity*> CreateProjectile(EntityManager* em,
                                              EProjectileType projectile_type,
                                              const CreateEntityOptions& options,
                                              EntityID target);

}  // namespace kdk

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
    float MoveSpeed = 8.0f;
    float Damage = 15.0f;
    EntityID Target = {};
    EProjectileType Type = EProjectileType::Invalid;
};
inline void Validate(const Scene*, const ProjectileEntity&, FixedVector<ValidationError, 64>*) {}

void Update(ProjectileEntity* projectile, float dt);
inline void Serialize(SerdeArchive*, ProjectileEntity*) {}

std::pair<EntityID, ProjectileEntity*> CreateProjectile(EntityManager* em,
                                                        EProjectileType projectile_type,
                                                        const CreateEntityOptions& options,
                                                        EntityID target);

}  // namespace kdk

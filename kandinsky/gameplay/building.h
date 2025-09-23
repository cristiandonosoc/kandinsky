#pragma once

#include <kandinsky/entity.h>

namespace kdk {

enum class EBuildingType : u8 {
    Invalid = 0,
    Base,
    COUNT,
};

struct BuildingEntity {
    GENERATE_ENTITY(Building);

    EBuildingType Type = EBuildingType::Invalid;

	float ShootInterval = 0.5f;
	float LastShot = 0.0f;
};

void Update(Entity* entity, BuildingEntity* building, float dt);
inline void Serialize(SerdeArchive*, BuildingEntity*) {}

std::pair<EntityID, Entity*> CreateBuilding(EntityManager* em,
                                            EBuildingType building_type,
                                            const CreateEntityOptions& options);

}  // namespace kdk

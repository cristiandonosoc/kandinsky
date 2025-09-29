#pragma once

#include <kandinsky/entity.h>

namespace kdk {

enum class EBuildingType : u8 {
    Invalid = 0,
    Tower,
    Base,
    COUNT,
};

struct BuildingEntity {
    GENERATE_ENTITY(Building);

    EBuildingType Type = EBuildingType::Invalid;

    float ShootInterval = 0.5f;
    float LastShot = 0.0f;
};

void Update(BuildingEntity* building, float dt);
inline void Serialize(SerdeArchive*, BuildingEntity*) {}
void BuildImGui(BuildingEntity* building);

std::pair<EntityID, BuildingEntity*> CreateBuilding(EntityManager* em,
                                                    EBuildingType building_type,
                                                    const CreateEntityOptions& options);

void Shoot(BuildingEntity* building);

}  // namespace kdk

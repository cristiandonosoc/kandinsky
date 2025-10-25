#pragma once

#include <kandinsky/entity.h>

namespace kdk {

enum class EBuildingType : u8 {
    Invalid = 0,
    Tower,
    Base,
    COUNT,
};
String ToString(EBuildingType building_type);

// #define BUILDING_ENTITY_FIELDS(X)                          \
//     X(EBuildingType, BuildingType, EBuildingType::Invalid) \
//     X(float, ShootInterval, 0.5f)                          \
//     X(float, LastShot, 0.0f)                               \
//     X(float, Lives, 10.0f)                                 \
//     X(float, Cost, 100.0f)

// DECLARE_ENTITY(Building, BUILDING_ENTITY_FIELDS)

struct BuildingEntity {
    GENERATE_ENTITY(Building)

    EBuildingType BuildingType = EBuildingType::Invalid;
    float ShootInterval = 0.5f;
    float LastShot = 0.0f;
    float Lives = 10.0f;
    float Cost = 100.0;
};

void Validate(const Scene*, const BuildingEntity&, FixedVector<ValidationError, 64>*);
void Serialize(SerdeArchive* sa, BuildingEntity* building);

void Update(BuildingEntity* building, float dt);
void BuildImGui(BuildingEntity* building);

std::pair<EntityID, BuildingEntity*> CreateBuilding(EntityManager* em,
                                                    EBuildingType building_type,
                                                    const CreateEntityOptions& options);

void Shoot(BuildingEntity* building);
void Hit(BuildingEntity* building, EnemyEntity* enemy);

}  // namespace kdk

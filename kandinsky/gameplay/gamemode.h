#pragma once

#include <kandinsky/entity.h>

namespace kdk {

struct BuildingEntity;

enum class EGameModeState : u8 {
    Idle = 0,
    PlacingBuilding,
};

struct GameMode {
    EGameModeState State = EGameModeState::Idle;
    EntityID Base = {};
    float Money = 1000.0f;
};

void Start(PlatformState* ps, GameMode* gm);
void Stop(PlatformState* ps, GameMode* gm);
void Update(PlatformState* ps, GameMode* gm, float dt);

void StartPlaceBuilding(GameMode* gm);
void BuildingDestroyed(GameMode* gm, BuildingEntity* building);

}  // namespace kdk

#pragma once

#include <kandinsky/entity.h>

namespace kdk {

struct BuildingEntity;

struct GameMode {
    EntityID Base = {};
    float Money = 1000.0f;
};

void Start(PlatformState* ps, GameMode* gm);
void Stop(PlatformState* ps, GameMode* gm);

void BuildingDestroyed(GameMode* gm, BuildingEntity* building);

}  // namespace kdk

#pragma once

#include <kandinsky/systems/system.h>

#include <kandinsky/systems/enemy_system.h>
#include <kandinsky/systems/schedule.h>

namespace kdk {

struct SystemManager {
#define X(ENUM_NAME, STRUCT_NAME, ...) STRUCT_NAME System_##ENUM_NAME = {};
    SYSTEM_TYPES(X)
#undef X
};

bool InitSystems(PlatformState* ps, SystemManager* sm);
void ShutdownSystems(SystemManager* sm);

void StartSystems(SystemManager* sm);
void StopSystems(SystemManager* sm);

void UpdateSystems(SystemManager* sm, float dt);

}  // namespace kdk

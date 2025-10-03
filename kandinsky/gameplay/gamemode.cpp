#include <kandinsky/gameplay/gamemode.h>

#include <kandinsky/gameplay/building.h>
#include <kandinsky/platform.h>

namespace kdk {

void BuildingDestroyed(GameMode* gm, BuildingEntity* building) {
    (void)gm;
    if (building->Type == EBuildingType::Base) {
        // Game over.
        auto* ps = platform::GetPlatformContext();
        ps->RunningMode = ERunningMode::GameEndRequested;
    }
}

}  // namespace kdk

#include <kandinsky/gameplay/gamemode.h>

#include <kandinsky/gameplay/building.h>
#include <kandinsky/platform.h>

namespace kdk {

void BuildingDestroyed(GameMode* gm, BuildingEntity* building) {
    (void)gm;
    if (building->BuildingType == EBuildingType::Base) {
        // Game over.
        auto* ps = platform::GetPlatformContext();
        ps->EditorState.RunningMode = ERunningMode::GameEndRequested;
    }
}

}  // namespace kdk

#include <kandinsky/gameplay/gamemode.h>

#include <kandinsky/gameplay/building.h>
#include <kandinsky/platform.h>

namespace kdk {

void Start(PlatformState* ps, GameMode* gm) {
    gm->Base = {};
    // Search for the base.
    // TODO(cdc): Perhaps this should come from the scene, since it does validation already?
    VisitEntities<BuildingEntity>(&ps->CurrentScene->EntityManager,
                                  [gm](EntityID id, BuildingEntity* building) {
                                      if (building->BuildingType == EBuildingType::Base) {
                                          gm->Base = id;
                                          return false;
                                      }

                                      return true;
                                  });

    // The scene should've been validated already.
    ASSERT(!IsNone(gm->Base));
}

void Stop(PlatformState* ps, GameMode* gm) {
    (void)ps;
    gm->Base = {};
}

void BuildingDestroyed(GameMode* gm, BuildingEntity* building) {
    (void)gm;
    if (building->BuildingType == EBuildingType::Base) {
        // Game over.
        auto* ps = platform::GetPlatformContext();
        ps->EditorState.RunningMode = ERunningMode::GameEndRequested;
    }
}

}  // namespace kdk

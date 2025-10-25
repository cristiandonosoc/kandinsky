#include <kandinsky/gameplay/gamemode.h>

#include <kandinsky/debug.h>
#include <kandinsky/gameplay/building.h>
#include <kandinsky/platform.h>
#include "kandinsky/entity.h"
#include "kandinsky/gameplay/terrain.h"

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

    gm->State = EGameModeState::Idle;
}

void Stop(PlatformState* ps, GameMode* gm) {
    (void)ps;
    gm->Base = {};
}

void Update(PlatformState* ps, GameMode* gm, float dt) {
    (void)dt;

    switch (gm->State) {
        case EGameModeState::Idle: break;
        case EGameModeState::PlacingBuilding: {
            if (auto result =
                    GetGridRayIntersection(*ps->CurrentCamera, ps->InputState.MousePosition);
                result) {
                constexpr i32 offset = 0;
                constexpr Vec3 extent = Vec3((float)offset + 0.5f, 0.0f, (float)offset + 0.5f);

                bool valid = true;
                ETerrainTileType type = GetTileSafe(ps->CurrentScene->Terrain, result->GridCoord);
                if (type != ETerrainTileType::Grass) {
                    valid = false;
                }

                EntityID placed_entity =
                    GetPlacedEntitySafe(ps->CurrentScene->Terrain, result->GridCoord);
                if (!IsNone(placed_entity)) {
                    valid = false;
                }

                Color32 color = valid ? Color32::Yellow : Color32::Red;
                Debug::DrawBox(ps, result->GridWorldLocation + Vec3(0, 0, 0), extent, color, 3);

                if (valid) {
                    if (MOUSE_PRESSED(ps, LEFT)) {
                        CreateEntityOptions options = {
                            .Transform = {result->GridWorldLocation + Vec3(0.0f, 0.5f, 0.0f)},
                            .Flags =
                                {
                                          .OnGrid = true,
                                          },
                        };
                        auto [id, _] =
                            CreateEntity<BuildingEntity>(&ps->CurrentScene->EntityManager, options);

                        PlaceEntity(&ps->CurrentScene->Terrain, id, result->GridCoord);

                        gm->State = EGameModeState::Idle;
                    }
                }
            }
        }
    }
}

void StartPlaceBuilding(GameMode* gm) {
    if (gm->State != EGameModeState::Idle) {
        return;
    }

    gm->State = EGameModeState::PlacingBuilding;
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

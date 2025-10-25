#include <kandinsky/scene.h>

#include <kandinsky/core/file.h>
#include <kandinsky/core/serde.h>
#include <kandinsky/platform.h>

namespace kdk {

void InitScene(Scene* scene, ESceneType scene_type) {
    scene->SceneType = scene_type;
    scene->EntityManager._OwnerScene = scene;
    InitTerrain(&scene->Terrain);
}

void StartScene(Scene* scene) { Start(&scene->EntityManager); }

void CloneScene(const Scene& src, Scene* dst) {
    dst->EntityManager = src.EntityManager;
    dst->EntityManager._OwnerScene = dst;
    dst->Terrain = src.Terrain;
}

void Serialize(SerdeArchive* sa, Scene* scene) {
    SERDE(sa, scene, Name);
    SERDE(sa, scene, EntityManager);
    SERDE(sa, scene, Terrain);
}

bool LoadScene(PlatformState* ps, const String& path) {
    ASSERT(ps->EditorState.RunningMode == ERunningMode::Editor);

    auto data = LoadFile(&ps->Memory.FrameArena, path, {.NullTerminate = false});
    if (data.empty()) {
        SDL_Log("Empty file read in %s", path.Str());
        return false;
    }

    SerdeArchive sa = NewSerdeArchive(&ps->Memory.PermanentArena,
                                      &ps->Memory.FrameArena,
                                      ESerdeBackend::YAML,
                                      ESerdeMode::Deserialize);
    Load(&sa, data);

    ResetStruct(&ps->EditorScene);

    SerdeContext sc = {};
    FillSerdeContext(ps, &sc);
    SetSerdeContext(&sa, &sc);
    Serde(&sa, "Scene", &ps->EditorScene);
    ps->EditorScene.Path = path;
    InitScene(&ps->EditorScene, ESceneType::Editor);

    return true;
}

// VALIDATION --------------------------------------------------------------------------------------

namespace scene_private {

// We will use soon.
struct ValidationContext {};

void ProcessValidationContext(Scene* scene, const ValidationContext& ctx) {
    (void)ctx;
    if (IsNone(scene->BaseEntityID)) {
        scene->ValidationErrors.Push({
            .Message = String("No base entity found in scene!"sv),
        });
    }

    scene->EntitiesWithErrors.Sort();
}

}  // namespace scene_private

bool ValidateScene(Scene* scene) {
    using namespace scene_private;

    ValidationContext ctx = {};

    scene->BaseEntityID = {};
    scene->ValidationErrors.Clear();
    scene->EntitiesWithErrors.Clear();
    VisitAllEntities(&scene->EntityManager, [scene, &ctx](EntityID id, Entity* entity) -> bool {
        (void)ctx;
        if (!ValidateEntity(scene, *entity, &scene->ValidationErrors)) {
            scene->EntitiesWithErrors.Push(id);
        }

        if (id.GetEntityType() == EEntityType::Building) {
            BuildingEntity* building = GetTypedEntity<BuildingEntity>(&scene->EntityManager, id);
            ASSERT(building);

            if (building->BuildingType == EBuildingType::Base) {
                if (!IsNone(scene->BaseEntityID)) {
                    scene->ValidationErrors.Push({
                        .Message = String("Multiple base entities found in scene!"sv),
                        .Position = building->GetTransform().Position,
                        .EntityID = id,
                    });
                } else {
                    scene->BaseEntityID = id;
                }
            }
        }

        return true;
    });

    ProcessValidationContext(scene, ctx);
    return scene->ValidationErrors.IsEmpty();
}

bool EntityHasValidationError(const Scene& scene, EntityID id) {
    return scene.EntitiesWithErrors.Contains(id);
}

}  // namespace kdk

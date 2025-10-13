#include <kandinsky/scene.h>

#include <kandinsky/core/serde.h>

namespace kdk {

void Serialize(SerdeArchive* sa, Scene* scene) {
    SERDE(sa, scene, Name);
    SERDE(sa, scene, EntityManager);
    SERDE(sa, scene, Terrain);
}

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

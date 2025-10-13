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
}

void StartScene(Scene* scene) { Start(&scene->EntityManager); }

void CloneScene(const Scene& src, Scene* dst) {
    dst->EntityManager = src.EntityManager;
    dst->EntityManager._OwnerScene = dst;
    dst->Terrain = src.Terrain;
}

// VALIDATION --------------------------------------------------------------------------------------

namespace scene_private {

struct ValidationContext {
    bool HasBase = false;
};

void ProcessValidationContext(Scene* scene, const ValidationContext& ctx) {
    if (!ctx.HasBase) {
        scene->ValidationErrors.Push({
            .Message = String("No base entity found in scene!"sv),
        });
    }
}

}  // namespace scene_private

bool ValidateScene(Scene* scene) {
    using namespace scene_private;

    ValidationContext ctx = {};

    scene->ValidationErrors.Clear();
    VisitAllEntities(&scene->EntityManager, [scene, &ctx](EntityID id, Entity* entity) -> bool {
        (void)id;
        Validate(scene, entity, &scene->ValidationErrors);

        if (id.GetEntityType() == EEntityType::Building) {
            BuildingEntity* building = GetTypedEntity<BuildingEntity>(&scene->EntityManager, id);
            ASSERT(building);

            if (building->BuildingType == EBuildingType::Base) {
                ctx.HasBase = true;
            }
        }

        return true;
    });

    ProcessValidationContext(scene, ctx);
    return scene->ValidationErrors.IsEmpty();
}

}  // namespace kdk

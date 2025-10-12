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

bool ValidateScene(Scene* scene) {
    scene->ValidationErrors.Clear();

    bool ok = true;
    VisitAllEntities(&scene->EntityManager, [scene, &ok](EntityID id, Entity* entity) -> bool {
        (void)id;
        ok &= Validate(scene, entity, &scene->ValidationErrors);
        return true;
    });

    return ok;
}

}  // namespace kdk

#include <kandinsky/scene.h>

#include <kandinsky/core/serde.h>

namespace kdk {

void Serialize(SerdeArchive* sa, Scene* scene) {
    SERDE(sa, scene, Name);
    SERDE(sa, scene, EntityManager);
}

void InitScene(Scene* scene, ESceneType scene_type) {
    scene->SceneType = scene_type;
    scene->EntityManager._OwnerScene = scene;
}

void StartScene(Scene* scene) { Start(&scene->EntityManager); }

}  // namespace kdk

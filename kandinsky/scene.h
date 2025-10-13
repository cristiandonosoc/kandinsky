#pragma once

#include <kandinsky/core/string.h>
#include <kandinsky/entity_manager.h>
#include <kandinsky/gameplay/terrain.h>

namespace kdk {

struct SerdeArchive;

enum class ESceneType : u8 {
    Invalid = 0,
    Editor,
    Game,
};

struct Scene {
	static constexpr i32 kMaxValidationErrors = 64;

	// TODO(cdc): Put all "clonable" data in it's own struct.
	//            That way we don't have to remember what has to be cloned in CloneScene.

    FixedString<128> Name = {};
    FixedString<512> Path = {};
    ESceneType SceneType = ESceneType::Invalid;

    EntityManager EntityManager = {};
    Terrain Terrain = {};

    FixedVector<ValidationError, 64> ValidationErrors = {};
	FixedVector<EntityID, 64> EntitiesWithErrors = {};
};


void InitScene(Scene* scene, ESceneType scene_type);
void StartScene(Scene* scene);

void Serialize(SerdeArchive* sa, Scene* scene);
void CloneScene(const Scene& src, Scene* dst);

// VALIDATION --------------------------------------------------------------------------------------

bool ValidateScene(Scene* scene);
inline bool HasValidationErrors(const Scene& scene) { return !scene.ValidationErrors.IsEmpty(); }
bool EntityHasValidationError(const Scene& scene, EntityID id);

}  // namespace kdk

#pragma once

#include <kandinsky/core/string.h>
#include <kandinsky/entity_manager.h>

namespace kdk {

struct SerdeArchive;

enum class ESceneType : u8 {
    Invalid = 0,
    Editor,
    Game,
};

struct Scene {
    FixedString<128> Name = {};
    FixedString<512> Path = {};
    ESceneType SceneType = ESceneType::Invalid;
    EntityManager EntityManager = {};
};

void InitScene(Scene* scene, ESceneType scene_type);
void StartScene(Scene* scene);

void Serialize(SerdeArchive* sa, Scene* scene);

}  // namespace kdk

#pragma once

#include <kandinsky/camera.h>
#include <kandinsky/game/entity.h>
#include <kandinsky/graphics/light.h>
#include <kandinsky/graphics/opengl.h>
#include <kandinsky/graphics/render_state.h>
#include <kandinsky/math.h>

#include <bitset>

namespace kdk {

struct GameState {
    Vec3 ClearColor = Vec3(0.2f);

    bool MainCameraMode = true;
    Camera MainCamera = {};
    Camera DebugCamera = {};
    Camera* CurrentCamera = nullptr;

    EntityManager EntityManager = {};
    EntityPicker EntityPicker = {};

    // Lights.
    Entity DirectionalLight = NONE;
    Entity PointLights[kNumPointLights] = {};
    Entity Spotlight = NONE;

	FixedArray<Entity, 32> Boxes = {};

	Entity SelectedEntity = NONE;
	Entity HoverEntity = NONE;

    std::array<Model*, 64> MiniDungeonModels = {};
    u32 MiniDungeonModelCount = 0;

    GLuint DebugFBO = NULL;
    GLuint DebugFBOTexture = NULL;
    GLuint DebugFBODepthStencil = NULL;
};

}  // namespace kdk

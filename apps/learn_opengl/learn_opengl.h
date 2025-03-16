#pragma once

#include <kandinsky/camera.h>
#include <kandinsky/entity.h>
#include <kandinsky/graphics/light.h>
#include <kandinsky/graphics/opengl.h>
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

    // Lights.
    Light DirectionalLight = {
        .LightType = ELightType::Directional,
    };
    Light PointLights[kNumPointLights] = {};
    Light Spotlight = {
        .LightType = ELightType::Spotlight,
    };

    std::array<Model*, 64> MiniDungeonModels = {};
    u32 MiniDungeonModelCount = 0;

    GLuint SSBO = NULL;
};

}  // namespace kdk

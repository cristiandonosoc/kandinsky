#pragma once

#include <kandinsky/camera.h>
#include <kandinsky/entity.h>
#include <kandinsky/entity_manager.h>
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
    DirectionalLight DirectionalLight = {};
    PointLight PointLights[kNumPointLights] = {};
    Spotlight Spotlight = {};

    std::array<Model*, 64> MiniDungeonModels = {};
    u32 MiniDungeonModelCount = 0;

    GLuint DebugFBO = NULL;
    GLuint DebugFBOTexture = NULL;
    GLuint DebugFBODepthStencil = NULL;

    GLuint SSBO = NULL;
};

}  // namespace kdk

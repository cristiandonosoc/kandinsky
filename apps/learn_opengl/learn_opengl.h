#pragma once

#include <kandinsky/entity.h>
#include <kandinsky/graphics/light.h>
#include <kandinsky/graphics/opengl.h>
#include <kandinsky/math.h>

#include <bitset>

namespace kdk {

struct GameState {
    Vec3 ClearColor = Vec3(0.2f);

    Camera FreeCamera = {};

    EntityManager EntityManager = {};

    // Lights.
    DirectionalLight DirectionalLight = {};
    PointLight PointLights[kNumPointLights] = {};
    Spotlight Spotlight = {};

    std::array<Model*, 64> MiniDungeonModels = {};
    u32 MiniDungeonModelCount = 0;

    GLuint SSBO = NULL;
};

}  // namespace kdk

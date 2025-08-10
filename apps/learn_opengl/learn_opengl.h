#pragma once

#include <kandinsky/camera.h>
#include <kandinsky/entity.h>
#include <kandinsky/graphics/light.h>
#include <kandinsky/graphics/model.h>
#include <kandinsky/graphics/opengl.h>
#include <kandinsky/graphics/render_state.h>
#include <kandinsky/math.h>

namespace kdk {

struct GameState {
    // Lights.
    DirectionalLightComponent* DirectionalLight = nullptr;
    PointLightComponent* PointLights[kNumPointLights] = {};
    SpotlightComponent* Spotlight = nullptr;

    FixedArray<EntityID, 32> Boxes = {};

    Material* BoxMaterial = nullptr;

    Model* BackpackModel = nullptr;
    std::array<Model*, 64> MiniDungeonModels = {};
    u32 MiniDungeonModelCount = 0;
};

}  // namespace kdk

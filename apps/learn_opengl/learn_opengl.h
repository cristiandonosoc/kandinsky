#pragma once

#include <kandinsky/camera.h>
#include <kandinsky/core/math.h>
#include <kandinsky/entity.h>
#include <kandinsky/graphics/light.h>
#include <kandinsky/graphics/model.h>
#include <kandinsky/graphics/opengl.h>
#include <kandinsky/graphics/render_state.h>

namespace kdk {

struct GameState {
    // Lights.
    DirectionalLightComponent* DirectionalLight = nullptr;
    PointLightComponent* PointLights[kNumPointLights] = {};
    SpotlightComponent* Spotlight = nullptr;

    FixedArray<EntityID, 32> Boxes = {};

    Material* BoxMaterial = nullptr;

    ModelAssetHandle BackpackModelHandle = {};
    FixedArray<ModelAssetHandle, 64> MiniDungeonModelHandles = {};
};

}  // namespace kdk

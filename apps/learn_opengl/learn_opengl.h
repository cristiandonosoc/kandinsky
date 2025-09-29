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

    FixedVector<EntityID, 32> Boxes = {};

    MaterialAssetHandle BoxMaterialHandle = {};

    ModelAssetHandle BackpackModelHandle = {};
    FixedVector<ModelAssetHandle, 64> MiniDungeonModelHandles = {};
};

}  // namespace kdk

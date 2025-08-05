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
    Vec3 ClearColor = Vec3(0.2f);

    bool MainCameraMode = true;
    Camera MainCamera = {};
    Camera DebugCamera = {};
    Camera* CurrentCamera = nullptr;

    EntityManager EntityManager = {};
    EntityPicker EntityPicker = {};

    // Lights.
    DirectionalLightComponent* DirectionalLight = nullptr;
    PointLightComponent* PointLights[kNumPointLights] = {};
    SpotlightComponent* Spotlight = nullptr;

    FixedArray<EntityID, 32> Boxes = {};

    EntityID SelectedEntityID = {};
    EntityID HoverEntityID = {};

	Material* BoxMaterial = nullptr;

    Shader* NormalShader = nullptr;
    Shader* LightShader = nullptr;
    Shader* LineBatcherShader = nullptr;

    Model* BackpackModel = nullptr;
    Model* SphereModel = nullptr;
    std::array<Model*, 64> MiniDungeonModels = {};
    u32 MiniDungeonModelCount = 0;

    GLuint DebugFBO = NULL;
    GLuint DebugFBOTexture = NULL;
    GLuint DebugFBODepthStencil = NULL;
};

}  // namespace kdk

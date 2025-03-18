#include <tower_defense/tower_defense.h>

#include <kandinsky/debug.h>
#include <kandinsky/glew.h>
#include <kandinsky/graphics/render_state.h>
#include <kandinsky/platform.h>
#include "kandinsky/entity.h"

namespace kdk {

namespace tower_defense_private {

TowerDefense* gTowerDefense = nullptr;

}  // namespace tower_defense_private

TowerDefense* TowerDefense::GetTowerDefense() { return tower_defense_private::gTowerDefense; }

bool TowerDefense::OnSharedObjectLoaded(PlatformState* ps) {
    platform::SetPlatformContext(ps);
    tower_defense_private::gTowerDefense = (TowerDefense*)ps->GameState;
    EntityManager::Set(&tower_defense_private::gTowerDefense->EntityManager);
    SDL_GL_MakeCurrent(ps->Window.SDLWindow, ps->Window.GLContext);

    // Initialize GLEW.
    if (!InitGlew(ps)) {
        return false;
    }

    ImGui::SetCurrentContext(ps->Imgui.Context);
    ImGui::SetAllocatorFunctions(ps->Imgui.AllocFunc, ps->Imgui.FreeFunc);

    SDL_Log("Game DLL Loaded");

    return true;
}

bool TowerDefense::OnSharedObjectUnloaded(PlatformState*) { return true; }

// Init --------------------------------------------------------------------------------------------

namespace tower_defense_private {

void InitCamera(PlatformState* ps, TowerDefense* td) {
    td->MainCamera.CameraType = ECameraType::Target;
    td->MainCamera.Position = Vec3(2, 2, 2);
    td->MainCamera.TargetCamera.Target = Vec3(0);
    td->DebugCamera.CameraType = ECameraType::Free;
    td->DebugCamera.FreeCamera = {};

    float aspect_ratio = (float)(ps->Window.Width) / (float)(ps->Window.Height);
	float zoom = 5;
    SetProjection(&td->MainCamera, Ortho(zoom * aspect_ratio, zoom, 0.1f, 10.f));
    SetProjection(&td->DebugCamera, Perspective(ToRadians(45.0f), aspect_ratio, 0.1f, 150.0f));

    glGenFramebuffers(1, &td->CameraFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, td->CameraFBO);

    glGenTextures(1, &td->CameraFBOTexture);
    glBindTexture(GL_TEXTURE_2D, td->CameraFBOTexture);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGB,
                 ps->Window.Width,
                 ps->Window.Height,
                 0,
                 GL_RGB,
                 GL_UNSIGNED_BYTE,
                 NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D,
                           td->CameraFBOTexture,
                           0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenRenderbuffers(1, &td->CameraFBODepthStencil);
    glBindRenderbuffer(GL_RENDERBUFFER, td->CameraFBODepthStencil);
    glRenderbufferStorage(GL_RENDERBUFFER,
                          GL_DEPTH24_STENCIL8,
                          ps->Window.Width,
                          ps->Window.Height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                              GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER,
                              td->CameraFBODepthStencil);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool InitShaders(PlatformState* ps) {
    auto scratch = GetScratchArena();

    {
        String vs_path =
            paths::PathJoin(scratch.Arena, ps->BasePath, String("assets/shaders/shader.vert"));
        String fs_path =
            paths::PathJoin(scratch.Arena, ps->BasePath, String("assets/shaders/shader.frag"));
        if (!CreateShader(&ps->Shaders.Registry, "NormalShader", vs_path.Str(), fs_path.Str())) {
            return false;
        }
    }

    {
        String vs_path =
            paths::PathJoin(scratch.Arena, ps->BasePath, String("assets/shaders/light.vert"));
        String fs_path =
            paths::PathJoin(scratch.Arena, ps->BasePath, String("assets/shaders/light.frag"));
        if (!CreateShader(&ps->Shaders.Registry, "LightShader", vs_path.Str(), fs_path.Str())) {
            return false;
        }
    }

    {
        String vs_path = paths::PathJoin(scratch.Arena,
                                         ps->BasePath,
                                         String("assets/shaders/line_batcher.vert"));
        String fs_path = paths::PathJoin(scratch.Arena,
                                         ps->BasePath,
                                         String("assets/shaders/line_batcher.frag"));
        if (!CreateShader(&ps->Shaders.Registry,
                          "LineBatcherShader",
                          vs_path.Str(),
                          fs_path.Str())) {
            return false;
        }
    }

    return true;
}

bool InitLights(PlatformState* ps, TowerDefense* td) {
    (void)ps;

    if (Light* dl = AddEntity<Light>(&td->EntityManager)) {
        FillEntity(dl, td->DirectionalLight);
    }

    return true;
}

}  // namespace tower_defense_private

bool TowerDefense::GameInit(PlatformState* ps) {
    using namespace tower_defense_private;

    TowerDefense* td = ArenaPush<TowerDefense>(platform::GetPermanentArena());
    *td = {};

    InitEntityManager(platform::GetPermanentArena(), &td->EntityManager);

    InitCamera(ps, td);

    for (u32 i = 0; i < kTileChunkSize; i++) {
        SetTile(&td->TileChunk, i, 0, ETileType::Grass);
        SetTile(&td->TileChunk, 0, i, ETileType::Grass);
        SetTile(&td->TileChunk, i, i, ETileType::Road);
    }

    td->Materials[(u32)ETileType::Grass] = Material{
        .Albedo = ToVec3(Color32::DarkGreen),
        .Diffuse = ToVec3(Color32::DarkGreen),
    };

    td->Materials[(u32)ETileType::Road] = Material{
        .Albedo = ToVec3(Color32::LightWood),
        .Diffuse = ToVec3(Color32::LightWood),
    };

    if (!InitShaders(ps)) {
        return false;
    }

    if (!InitLights(ps, td)) {
        return false;
    }

    ps->GameState = td;
    tower_defense_private::gTowerDefense = td;

    return true;
}

// UPDATE ------------------------------------------------------------------------------------------

namespace tower_defense_private {

void BuildImgui(PlatformState* ps, TowerDefense* td) {
    (void)ps;

    ImGui::Begin("Tower Defense");
    if (!td->MainCameraMode) {
        ImGui::Text("DEBUG CAMERA");
    }

    if (ImGui::TreeNodeEx("Camera", ImGuiTreeNodeFlags_Framed)) {
        BuildImgui(&td->MainCamera, td->MainCameraMode ? NULL : td->CameraFBOTexture);
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Debug Camera", ImGuiTreeNodeFlags_Framed)) {
        BuildImgui(&td->DebugCamera);
        ImGui::TreePop();
    }

    ImGui::End();
}

}  // namespace tower_defense_private

bool TowerDefense::GameUpdate(PlatformState* ps) {
    using namespace tower_defense_private;

    auto* td = GetTowerDefense();

    if (KEY_PRESSED(ps, SPACE)) {
        td->MainCameraMode = !td->MainCameraMode;
        SetupDebugCamera(td->MainCamera, &td->DebugCamera);
    }

    Recalculate(&td->MainCamera);
    Recalculate(&td->DebugCamera);
    if (td->MainCameraMode) {
        Update(ps, &td->MainCamera, ps->FrameDelta);
    } else {
        Update(ps, &td->DebugCamera, ps->FrameDelta);
    }

    BuildImgui(ps, td);

    return true;
}

// Render ------------------------------------------------------------------------------------------

namespace tower_defense_private {

struct RenderSceneOptions {
    Camera* DebugCameraToRender = nullptr;
    bool RenderDebug = true;
};

void RenderScene(PlatformState* ps,
                 TowerDefense* td,
                 const Camera& camera,
                 const RenderSceneOptions& options = {}) {
    Shader* light_shader = FindShader(&ps->Shaders.Registry, "LightShader");
    ASSERT(light_shader);
    Shader* line_batcher_shader = FindShader(&ps->Shaders.Registry, "LineBatcherShader");
    ASSERT(line_batcher_shader);

    RenderState rs = {};
    SetCamera(&rs, camera);

    std::array<Light*, 16> lights = {};
    u32 light_count = 0;
    for (auto it = GetEntityIterator<Light>(&td->EntityManager); it; it++) {
        ASSERT(light_count < lights.size());
        lights[light_count++] = &it.Get();
    }
    SetLights(&rs, {lights.data(), light_count});

    glViewport(0, 0, ps->Window.Width, ps->Window.Height);

    glEnable(GL_DEPTH_TEST);

    glClearColor(0.3f, 0.3f, 0.6f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    DrawGrid(rs);

    Shader* normal_shader = FindShader(&ps->Shaders.Registry, "NormalShader");
    ASSERT(normal_shader);

    Mesh* cube_mesh = FindMesh(&ps->Meshes, "Cube");
    ASSERT(cube_mesh);
    for (u32 z = 0; z < kTileChunkSize; z++) {
        for (u32 x = 0; x < kTileChunkSize; x++) {
            ETileType tile = GetTile(td->TileChunk, x, z);
            if (tile == ETileType::None) {
                continue;
            }

            Material& material = td->Materials[(u32)tile];

            Mat4 mmodel(1.0f);
            mmodel = Translate(mmodel, Vec3(x, 0, z));
            mmodel = Scale(mmodel, Vec3(0.9f));

            ChangeModelMatrix(&rs, mmodel);
            Draw(*cube_mesh, *normal_shader, rs, &material);
        }
    }

    // Draw the camera.
    if (options.DebugCameraToRender) {
        Use(*light_shader);

        Mat4 mmodel(1.0f);
        mmodel = Translate(mmodel, options.DebugCameraToRender->Position);
        mmodel = Scale(mmodel, Vec3(0.1f));
        ChangeModelMatrix(&rs, mmodel);

        Color32 color = Color32::MandarianOrange;
        SetVec3(*light_shader, "uColor", ToVec3(color));
        Draw(*cube_mesh, *light_shader, rs);
        DrawDebug(ps, *options.DebugCameraToRender, color);
    }

    if (options.RenderDebug) {
        Debug::Render(ps, *line_batcher_shader, camera.M_ViewProj);
    }
}

}  // namespace tower_defense_private

bool TowerDefense::GameRender(PlatformState* ps) {
    using namespace tower_defense_private;

    auto* td = GetTowerDefense();

    if (td->MainCameraMode) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        RenderScene(ps, td, td->MainCamera);
    } else {
        RenderSceneOptions options = {
            .RenderDebug = false,
        };

        glBindFramebuffer(GL_FRAMEBUFFER, td->CameraFBO);
        RenderScene(ps, td, td->MainCamera, options);

        options.DebugCameraToRender = &td->MainCamera;
        options.RenderDebug = true;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        RenderScene(ps, td, td->DebugCamera, options);
    }

    ps->Functions.RenderImgui();

    return true;
}

}  // namespace kdk

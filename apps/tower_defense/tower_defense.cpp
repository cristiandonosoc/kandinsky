#include <tower_defense/tower_defense.h>

#include <imgui.h>
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
    td->MainCamera.WindowSize = {ps->Window.Width, ps->Window.Height};
    td->MainCamera.CameraType = ECameraType::Target;
    td->MainCamera.TargetCamera = {};
    td->MainCamera.ProjectionType = ECameraProjectionType::Ortho;
    td->MainCamera.OrthoData = {};

    td->DebugCamera.WindowSize = {ps->Window.Width, ps->Window.Height};
    td->DebugCamera.CameraType = ECameraType::Free;
    td->DebugCamera.FreeCamera = {};
    td->DebugCamera.ProjectionType = ECameraProjectionType::Perspective;
    td->DebugCamera.PerspectiveData = {};

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
        ImGui::Checkbox("Update Main Camera instead", &td->UpdateMainCameraOnDebugMode);
    }

    if (ImGui::TreeNodeEx("Camera", ImGuiTreeNodeFlags_Framed)) {
        BuildImgui(&td->MainCamera, td->MainCameraMode ? NULL : td->CameraFBOTexture);
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Debug Camera", ImGuiTreeNodeFlags_Framed)) {
        BuildImgui(&td->DebugCamera);
        ImGui::TreePop();
    }

    // Add tile type selection
    if (ImGui::TreeNodeEx("Tile Placement", ImGuiTreeNodeFlags_Framed)) {
        const char* tile_types[] = {"Grass", "Road"};
        int current_type = (int)td->SelectedTileType - 1; // -1 because None is 0
        if (ImGui::Combo("Tile Type", &current_type, tile_types, IM_ARRAYSIZE(tile_types))) {
            td->SelectedTileType = (ETileType)(current_type + 1);
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Tile Grid", ImGuiTreeNodeFlags_Framed)) {
        const float square_size = 20.0f;  // Size of each grid square in pixels
        const float grid_spacing = 2.0f;  // Spacing between squares
        
        ImVec2 cursor = ImGui::GetCursorScreenPos();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        
        // Draw the grid
        for (u32 z = 0; z < kTileChunkSize; z++) {
            for (u32 x = 0; x < kTileChunkSize; x++) {
                ETileType tile = GetTile(td->TileChunk, x, z);
                
                ImVec2 square_min = ImVec2(
                    cursor.x + x * (square_size + grid_spacing),
                    cursor.y + z * (square_size + grid_spacing)
                );
                ImVec2 square_max = ImVec2(
                    square_min.x + square_size,
                    square_min.y + square_size
                );

                // Choose color based on tile type
                ImU32 color;
                const char* tooltip = "None";
                switch (tile) {
                    case ETileType::Grass:
                        color = IM_COL32(0, 150, 0, 255);  // Dark green
                        tooltip = "Grass";
                        break;
                    case ETileType::Road:
                        color = IM_COL32(139, 69, 19, 255);  // Brown
                        tooltip = "Road";
                        break;
                    default:
                        color = IM_COL32(50, 50, 50, 255);  // Dark gray
                        break;
                }

                // Draw the square
                draw_list->AddRectFilled(square_min, square_max, color);

                // Add tooltip
                if (ImGui::IsMouseHoveringRect(square_min, square_max)) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Tile (%d, %d): %s", x, z, tooltip);
                    ImGui::EndTooltip();
                }
            }
        }

        // Add enough vertical space for the grid
        ImGui::Dummy(ImVec2(
            kTileChunkSize * (square_size + grid_spacing),
            kTileChunkSize * (square_size + grid_spacing)
        ));

        ImGui::TreePop();
    }

    ImGui::End();
}

}  // namespace tower_defense_private

bool TowerDefense::GameUpdate(PlatformState* ps) {
    using namespace tower_defense_private;

    auto scratch = GetScratchArena();

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
        if (!td->UpdateMainCameraOnDebugMode) {
            Update(ps, &td->DebugCamera, ps->FrameDelta);
        } else {
            Update(ps, &td->MainCamera, ps->FrameDelta);
        }
    }
    auto* current_camera = td->MainCameraMode ? &td->MainCamera : &td->DebugCamera;

    BuildImgui(ps, td);

    Plane base_plane{
        .Normal = Vec3(0, 1, 0),
    };

    auto [ray_pos, ray_dir] = GetWorldRay(*current_camera, ps->InputState.MousePosition);

    // SDL_Log("Mouse pos: %s, Camera pos: %s, Ray pos: %s, Ray dir: %s",
    //         ToString(scratch.Arena, ps->InputState.MousePosition),
    //         ToString(scratch.Arena, current_camera->Position),
    //         ToString(scratch.Arena, ray_pos),
    //         ToString(scratch.Arena, ray_dir));

    Vec3 intersection = {};
    if (IntersectPlaneRay(base_plane, ray_pos, ray_dir, &intersection)) {
        Debug::DrawSphere(ps, intersection, 0.05f, 16, Color32::Yellow);

        // Get the coordinate
        Vec3 coord = Round(intersection);
        
        // Add tile placement on click
        if (MOUSE_PRESSED(ps, LEFT)) {
            SDL_Log("Placing tile at: %s", ToString(scratch.Arena, coord));
            int x = (int)coord.x;
            int z = (int)coord.z;
            if (x >= 0 && x < kTileChunkSize && z >= 0 && z < kTileChunkSize) {
                SetTile(&td->TileChunk, x, z, td->SelectedTileType);
            }
        }

        // SDL_Log("Intersection: %s, Coord: %s",
        //         ToString(scratch.Arena, intersection),
        //         ToString(scratch.Arena, coord));
        Debug::DrawBox(ps, coord + Vec3(0, -0.5f, 0), Vec3(0.5f), Color32::Yellow, 3);
        // Debug::DrawBox(ps, coord - Vec3(0, 0, 0), Vec3(0.5f), Color32::Yellow, 3);
    }

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

    DrawGrid(rs, 50.0f, 100.0f);

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
            mmodel = Translate(mmodel, Vec3(x, -0.5f, z));
            // mmodel = Translate(mmodel, Vec3(x, 0, z));
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

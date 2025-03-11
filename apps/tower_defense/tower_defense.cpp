#include <tower_defense/tower_defense.h>

#include <kandinsky/glew.h>
#include <kandinsky/graphics/render_state.h>
#include <kandinsky/platform.h>

namespace kdk {

namespace tower_defense_private {

TowerDefense* gTowerDefense = nullptr;

}  // namespace tower_defense_private

TowerDefense* TowerDefense::GetTowerDefense() { return tower_defense_private::gTowerDefense; }

bool TowerDefense::OnSharedObjectLoaded(PlatformState* ps) {
    platform::SetPlatformContext(ps);
    SDL_GL_MakeCurrent(ps->Window.SDLWindow, ps->Window.GLContext);

    // Initialize GLEW.
    if (!InitGlew(ps)) {
        return false;
    }

    ImGui::SetCurrentContext(ps->Imgui.Context);
    ImGui::SetAllocatorFunctions(ps->Imgui.AllocFunc, ps->Imgui.FreeFunc);

    tower_defense_private::gTowerDefense = (TowerDefense*)ps->GameState;

    SDL_Log("Game DLL Loaded");

    return true;
}

bool TowerDefense::OnSharedObjectUnloaded(PlatformState*) { return true; }

bool TowerDefense::GameInit(PlatformState* ps) {
    auto scratch = GetScratchArena();

    TowerDefense* td = ArenaPush<TowerDefense>(platform::GetPermanentArena());
    *td = {};

    td->Camera.CameraType = ECameraType::Target;
    td->Camera.Position = Vec3(2, 2, 2);
    td->Camera.TargetCamera.Target = Vec3(0);

    float aspect_ratio = (float)(ps->Window.Width) / (float)(ps->Window.Height);
    SetProjection(&td->Camera, Ortho(10 * aspect_ratio, 10, 0.1f, 100.f));

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

    ps->GameState = td;
    tower_defense_private::gTowerDefense = td;

    {
        String vs_path =
            paths::PathJoin(scratch.Arena, ps->BasePath, String("assets/shaders/shader.vert"));
        String fs_path =
            paths::PathJoin(scratch.Arena, ps->BasePath, String("assets/shaders/shader.frag"));
        if (!CreateShader(&ps->Shaders.Registry, "NormalShader", vs_path.Str(), fs_path.Str())) {
            return false;
        }
    }

    return true;
}

bool TowerDefense::GameUpdate(PlatformState* ps) {
    auto* td = GetTowerDefense();

    Update(ps, &td->Camera, ps->FrameDelta);

    return true;
}

bool TowerDefense::GameRender(PlatformState* ps) {
    auto* td = GetTowerDefense();

    RenderState rs = {};
    SetCamera(&rs, td->Camera);
    rs.DirectionalLight.DL = &td->DirectionalLight.DirectionalLight;
    rs.DirectionalLight.ViewDirection = rs.M_View * Vec4(rs.DirectionalLight.DL->Direction, 0.0f);

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

    return true;
}

}  // namespace kdk

#include <learn_opengl/learn_opengl.h>

#include <kandinsky/debug.h>
#include <kandinsky/defines.h>
#include <kandinsky/glew.h>
#include <kandinsky/imgui.h>
#include <kandinsky/math.h>
#include <kandinsky/platform.h>
#include <kandinsky/print.h>
#include <kandinsky/utils/defer.h>
#include <kandinsky/window.h>

#include <SDL3/SDL_mouse.h>

#include <ImGuizmo.h>
#include <imgui.h>

#include <string>

#ifdef __cplusplus
extern "C" {
#endif

namespace kdk {

// clang-format off
std::array kVertices = {
    // positions          // normals           // texture coords
	Vertex{{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f,},  {0.0f, 0.0f}},
    Vertex{{ 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f,},  {1.0f, 0.0f}},
    Vertex{{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f,},  {1.0f, 1.0f}},
    Vertex{{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f,},  {1.0f, 1.0f}},
    Vertex{{-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f,},  {0.0f, 1.0f}},
    Vertex{{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f,},  {0.0f, 0.0f}},

    Vertex{{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f,},  {0.0f, 0.0f}},
    Vertex{{ 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f,},  {1.0f, 0.0f}},
    Vertex{{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f,},  {1.0f, 1.0f}},
    Vertex{{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f,},  {1.0f, 1.0f}},
    Vertex{{-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f,},  {0.0f, 1.0f}},
    Vertex{{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f,},  {0.0f, 0.0f}},

    Vertex{{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f,},  {1.0f, 0.0f}},
    Vertex{{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f,},  {1.0f, 1.0f}},
    Vertex{{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f,},  {0.0f, 1.0f}},
    Vertex{{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f,},  {0.0f, 1.0f}},
    Vertex{{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f,},  {0.0f, 0.0f}},
    Vertex{{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f,},  {1.0f, 0.0f}},

    Vertex{{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f,},  {1.0f, 0.0f}},
    Vertex{{ 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f,},  {1.0f, 1.0f}},
    Vertex{{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f,},  {0.0f, 1.0f}},
    Vertex{{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f,},  {0.0f, 1.0f}},
    Vertex{{ 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f,},  {0.0f, 0.0f}},
    Vertex{{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f,},  {1.0f, 0.0f}},

    Vertex{{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f,},  {0.0f, 1.0f}},
    Vertex{{ 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f,},  {1.0f, 1.0f}},
    Vertex{{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f,},  {1.0f, 0.0f}},
    Vertex{{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f,},  {1.0f, 0.0f}},
    Vertex{{-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f,},  {0.0f, 0.0f}},
    Vertex{{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f,},  {0.0f, 1.0f}},

    Vertex{{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f,},  {0.0f, 1.0f}},
    Vertex{{ 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f,},  {1.0f, 1.0f}},
    Vertex{{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f,},  {1.0f, 0.0f}},
    Vertex{{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f,},  {1.0f, 0.0f}},
    Vertex{{-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f,},  {0.0f, 0.0f}},
    Vertex{{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f,},  {0.0f, 1.0f}},
};

Vec3 kCubePositions[] = {Vec3(0.0f, 0.0f, 0.0f),
                         Vec3(2.0f, 5.0f, -15.0f),
                         Vec3(-1.5f, -2.2f, -2.5f),
                         Vec3(-3.8f, -2.0f, -12.3f),
                         Vec3(2.4f, -0.4f, -3.5f),
                         Vec3(-1.7f, 3.0f, -7.5f),
                         Vec3(1.3f, -2.0f, -2.5f),
                         Vec3(1.5f, 2.0f, -2.5f),
                         Vec3(1.5f, 0.2f, -1.5f),
                         Vec3(-1.3f, 1.0f, -1.5f)};
// clang-format on

bool OnSharedObjectLoaded(PlatformState* ps) {
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

bool OnSharedObjectUnloaded(PlatformState*) {
    SDL_Log("Game DLL Unloaded");
    return true;
}

bool GameInit(PlatformState* ps) {
    GameState* gs = (GameState*)ArenaPush(&ps->Memory.PermanentArena, sizeof(GameState));
    *gs = {};
    gs->FreeCamera.Position = Vec3(-4.0f, 1.0f, 1.0f);
    gs->DirectionalLight.Direction = Vec3(-1.0f, -1.0f, -1.0f);
    gs->DirectionalLight.Color = {
        .Ambient = Vec3(0.05f),
        .Diffuse = Vec3(0.4f),
        .Specular = Vec3(0.05f),
    };
    for (u64 i = 0; i < std::size(gs->PointLights); i++) {
        PointLight& pl = gs->PointLights[i];
        pl.Color = {.Ambient = Vec3(0.05f), .Diffuse = Vec3(0.8f), .Specular = Vec3(1.0f)};
    }
    gs->PointLights[0].Position = Vec3(0.7f, 0.2f, 2.0f);
    gs->PointLights[1].Position = Vec3(2.3f, -3.3f, -4.0f);
    gs->PointLights[2].Position = Vec3(-4.0f, 2.0f, -12.0f);
    gs->PointLights[3].Position = Vec3(0.0f, 0.0f, -3.0f);

    gs->Spotlight.Position = Vec3(-1.0f);
    gs->Spotlight.Target = Vec3(0);
    gs->Spotlight.Color = {.Ambient = Vec3(0.05f), .Diffuse = Vec3(0.8f), .Specular = Vec3(1.0f)};

    gs->Material.Shininess = 10.0f;

    ps->GameState = gs;

    {
        LineBatcher* grid_line_batcher =
            CreateLineBatcher(ps, &ps->LineBatchers, "GridLineBatcher");
        if (!IsValid(*grid_line_batcher)) {
            SDL_Log("ERROR: creating line batcher");
            return false;
        }

        StartLineBatch(grid_line_batcher);

        constexpr i32 kMeters = 100;
        for (i32 i = -kMeters; i <= kMeters; i++) {
            if (i == 0) {
                continue;
            }

            // clang-format off
            std::array<Vec3, 4> points{
                // X-axis.
                Vec3(i, 0, -kMeters),
                Vec3(i, 0, kMeters),
                // Z-Axis.
                Vec3(-kMeters, 0, i),
                Vec3( kMeters, 0, i),
            };
            // clang-format on

            AddPoints(grid_line_batcher, points);
        };

        EndLineBatch(grid_line_batcher);

        // X-Axis.
        StartLineBatch(grid_line_batcher, GL_LINES, Color32::Red, 3.0f);
        AddPoint(grid_line_batcher, {-kMeters, 0, 0});
        AddPoint(grid_line_batcher, {kMeters, 0, 0});
        EndLineBatch(grid_line_batcher);

        // Y-Axis.
        StartLineBatch(grid_line_batcher, GL_LINES, Color32::Green, 3.0f);
        AddPoint(grid_line_batcher, {0, -kMeters, 0});
        AddPoint(grid_line_batcher, {0, kMeters, 0});
        EndLineBatch(grid_line_batcher);

        // Z-Axis.
        StartLineBatch(grid_line_batcher, GL_LINES, Color32::Blue, 3.0f);
        AddPoint(grid_line_batcher, {0, 0, -kMeters});
        AddPoint(grid_line_batcher, {0, 0, kMeters});
        EndLineBatch(grid_line_batcher);

        Buffer(ps, *grid_line_batcher);
    }

    // Textures.

    std::string path;
    path = ps->BasePath + "assets/textures/container2.png";
    Texture* diffuse_texture = CreateTexture(ps,
                                             &ps->Textures,
                                             "DiffuseTexture",
                                             path.c_str(),
                                             {
                                                 .Type = ETextureType::Diffuse,

                                             });
    if (!diffuse_texture) {
        SDL_Log("ERROR: Loading diffuse texture");
        return false;
    }

    path = ps->BasePath + "assets/textures/container2_specular.png";
    Texture* specular_texture = CreateTexture(ps,
                                              &ps->Textures,
                                              "SpecularTexture",
                                              path.c_str(),
                                              {
                                                  .Type = ETextureType::Specular,
                                              });
    if (!specular_texture) {
        SDL_Log("ERROR: Loading specular texture");
        return false;
    }

    path = ps->BasePath + "assets/textures/matrix.jpg";
    Texture* emissive_texture = CreateTexture(ps,
                                              &ps->Textures,
                                              "EmissionTexture",
                                              path.c_str(),
                                              {
                                                  .Type = ETextureType::Emissive,
                                                  .WrapT = GL_MIRRORED_REPEAT,
                                              });
    if (!emissive_texture) {
        SDL_Log("ERROR: Loading emissive texture");
        return false;
    }

    // Meshes.
    {
        CreateMeshOptions options{
            .VerticesCount = (u32)kVertices.size(),
            .Vertices = kVertices.data(),
            .Textures = {diffuse_texture, specular_texture, emissive_texture},
        };

        if (!CreateMesh(ps, &ps->Meshes, "CubeMesh", options)) {
            return false;
        }
    }

    if (!CreateMesh(ps,
                    &ps->Meshes,
                    "LightMesh",
                    {
                        .VerticesCount = (u32)kVertices.size(),
                        .Vertices = kVertices.data(),
                    })) {
        return false;
    }

    // Shaders.

    {
        std::string vs_path = ps->BasePath + "assets/shaders/shader.vert";
        std::string fs_path = ps->BasePath + "assets/shaders/shader.frag";
        if (!CreateShader(ps,
                          &ps->Shaders.Registry,
                          "NormalShader",
                          vs_path.c_str(),
                          fs_path.c_str())) {
            return false;
        }
    }

    {
        std::string vs_path = ps->BasePath + "assets/shaders/light.vert";
        std::string fs_path = ps->BasePath + "assets/shaders/light.frag";
        if (!CreateShader(ps,
                          &ps->Shaders.Registry,
                          "LightShader",
                          vs_path.c_str(),
                          fs_path.c_str())) {
            return false;
        }
    }

    {
        std::string vs_path = ps->BasePath + "assets/shaders/line_batcher.vert";
        std::string fs_path = ps->BasePath + "assets/shaders/line_batcher.frag";
        if (!CreateShader(ps,
                          &ps->Shaders.Registry,
                          "LineBatcherShader",
                          vs_path.c_str(),
                          fs_path.c_str())) {
            return false;
        }
    }

    return true;
}

bool GameUpdate(PlatformState* ps) {
    ImGuizmo::BeginFrame();
    ImGuizmo::Enable(true);
    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    GameState* gs = (GameState*)ps->GameState;
    assert(gs);

    Update(ps, &gs->FreeCamera, ps->FrameDelta);

    if (ImGui::Begin("Kandinsky")) {
        ImGui::ColorEdit3("Clear Color", GetPtr(gs->ClearColor), ImGuiColorEditFlags_Float);

        if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::InputFloat3("Position", GetPtr(gs->FreeCamera.Position));
        }

        ImGui::DragFloat("Shininess", &gs->Material.Shininess, 1.0f, 0.0f, 200);

        if (ImGui::TreeNodeEx("Lights",
                              ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
            if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                BuildImGui(&gs->DirectionalLight);
            }

            // clang-format off
			if (ImGui::Button("SELECT PL0")) { gs->SelectedLight = 0; } ImGui::SameLine();
			if (ImGui::Button("SELECT PL1")) { gs->SelectedLight = 1; } ImGui::SameLine();
			if (ImGui::Button("SELECT PL2")) { gs->SelectedLight = 2; } ImGui::SameLine();
			if (ImGui::Button("SELECT PL3")) { gs->SelectedLight = 3; } ImGui::SameLine();
			if (ImGui::Button("SELECT SPOTLIGHT")) { gs->SelectedLight = 5; }
            // clang-format on

            for (u64 i = 0; i < std::size(gs->PointLights); i++) {
                const char* title = Printf(&ps->Memory.FrameArena, "Light %d", i);
                ImGui::PushID(title);
                if (ImGui::CollapsingHeader(title)) {
                    PointLight& pl = gs->PointLights[i];
                    BuildImGui(&pl);
                }
                ImGui::PopID();
            }

            if (ImGui::CollapsingHeader("Spotlight")) {
                BuildImgui(&gs->Spotlight);
            }

            ImGui::TreePop();
        }

        if (i32 selected_light = gs->SelectedLight; selected_light != NONE) {
            if (selected_light < (i32)std::size(gs->PointLights)) {
                PointLight& pl = gs->PointLights[gs->SelectedLight];
                Debug::DrawSphere(ps, pl.Position, pl.MinRadius, 16, Color32::Black);
                Debug::DrawSphere(ps, pl.Position, pl.MaxRadius, 16, Color32::Grey);

                Mat4 model(1.0f);
                model = glm::translate(model, Vec3(pl.Position));
                if (ImGuizmo::Manipulate(GetPtr(gs->FreeCamera.View),
                                         GetPtr(gs->FreeCamera.Proj),
                                         ImGuizmo::TRANSLATE,
                                         ImGuizmo::WORLD,
                                         GetPtr(model))) {
                    pl.Position = model[3];
                }
            } else if (selected_light == 5) {
                Spotlight& sl = gs->Spotlight;

                Vec3 direction = GetDirection(sl);
                Debug::DrawCone(ps,
                                sl.Position,
                                direction,
                                sl.MaxCutoffDistance,
                                glm::radians(sl.OuterRadiusDeg),
                                16,
                                Color32::Orange);

                Mat4 posmat(1.0f);
                posmat = glm::translate(posmat, Vec3(sl.Position));
                if (ImGuizmo::Manipulate(GetPtr(gs->FreeCamera.View),
                                         GetPtr(gs->FreeCamera.Proj),
                                         ImGuizmo::TRANSLATE,
                                         ImGuizmo::WORLD,
                                         GetPtr(posmat))) {
                    sl.Position = posmat[3];
                    Recalculate(&sl);
                }
            }
        }

        ImGui::End();
    }

#if 0
#endif

    return true;
}

bool GameRender(PlatformState* ps) {
    using namespace kdk;

    GameState* gs = (GameState*)ps->GameState;
    assert(gs);

    LineBatcher* grid_line_batcher = FindLineBatcher(&ps->LineBatchers, "GridLineBatcher");
    assert(grid_line_batcher);

    Mesh* cube_mesh = FindMesh(&ps->Meshes, "CubeMesh");
    assert(cube_mesh);
    Mesh* light_mesh = FindMesh(&ps->Meshes, "LightMesh");
    assert(light_mesh);

    Shader* normal_shader = FindShader(&ps->Shaders.Registry, "NormalShader");
    assert(normal_shader);
    Shader* light_shader = FindShader(&ps->Shaders.Registry, "LightShader");
    assert(light_shader);
    Shader* line_batcher_shader = FindShader(&ps->Shaders.Registry, "LineBatcherShader");
    assert(line_batcher_shader);

    // Texture* diffuse_texture = FindTexture(&ps->Textures, "DiffuseTexture");
    // assert(diffuse_texture);
    // Texture* specular_texture = FindTexture(&ps->Textures, "SpecularTexture");
    // assert(specular_texture);
    // Texture* emissive_texture = FindTexture(&ps->Textures, "EmissionTexture");
    // assert(emissive_texture);

    // Calculate the render state.
    RenderState rs = {};
    // rs.Seconds = 0;
    rs.Seconds = 0.5f * static_cast<float>(SDL_GetTicks()) / 1000.0f;
    rs.MatView = &gs->FreeCamera.View;
    rs.MatProj = &gs->FreeCamera.Proj;
    rs.MatViewProj = &gs->FreeCamera.ViewProj;
    rs.DirectionalLight.DL = &gs->DirectionalLight;
    rs.DirectionalLight.ViewDirection = (*rs.MatView) * Vec4(gs->DirectionalLight.Direction, 0.0f);
    for (u64 i = 0; i < std::size(gs->PointLights); i++) {
        rs.PointLights[i].PL = &gs->PointLights[i];
        rs.PointLights[i].ViewPosition = (*rs.MatView) * Vec4(gs->PointLights[i].Position, 1.0f);
    }
    rs.Spotlight.SL = &gs->Spotlight;
    rs.Spotlight.ViewPosition = (*rs.MatView) * Vec4(gs->Spotlight.Position, 1.0f);
    Vec3 spotlight_dir = gs->Spotlight.Target - gs->Spotlight.Position;
    rs.Spotlight.ViewDirection = (*rs.MatView) * Vec4(spotlight_dir, 0.0f);
    rs.Spotlight.InnerRadiusCos = glm::cos(glm::radians(gs->Spotlight.InnerRadiusDeg));
    rs.Spotlight.OuterRadiusCos = glm::cos(glm::radians(gs->Spotlight.OuterRadiusDeg));

    glViewport(0, 0, ps->Window.Width, ps->Window.Height);

    glEnable(GL_DEPTH_TEST);

    glClearColor(gs->ClearColor.r, gs->ClearColor.g, gs->ClearColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render plane.
    {
        Use(*line_batcher_shader);
        SetMat4(*line_batcher_shader, "uViewProj", GetPtr(gs->FreeCamera.ViewProj));
        Draw(*grid_line_batcher, *line_batcher_shader);
    }

    // Render cubes.
    {
        const Mat4& view = gs->FreeCamera.View;

        Use(*normal_shader);
        SetFloat(*normal_shader, "uMaterial.Shininess", gs->Material.Shininess);
        for (const auto& position : kCubePositions) {
            // TODO(cdc): These should be handled on update and just rendered here.
            Mat4 model = Mat4(1.0f);
            model = glm::translate(model, position);
            model = glm::rotate(model, glm::radians(rs.Seconds * 25), Vec3(1.0f, 0.0f, 0.0f));

            Mat4 view_model = view * model;
            Mat4 normal_matrix = glm::transpose(glm::inverse(view_model));

            SetMat4(*normal_shader, "uViewModel", GetPtr(view_model));
            SetMat4(*normal_shader, "uNormalMatrix", GetPtr(normal_matrix));

            DrawMesh(*cube_mesh, *normal_shader, rs);
        }
    }

    // Render Lights.
    {
        for (u64 i = 0; i < std::size(gs->PointLights); i++) {
            PointLight& pl = gs->PointLights[i];
            Draw(pl, *light_shader, *light_mesh, rs);
        }
    }

    kdk::Debug::Render(ps, *line_batcher_shader, gs->FreeCamera.ViewProj);

    return true;
}

}  // namespace kdk

#ifdef __cplusplus
}
#endif

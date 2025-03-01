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
    SetPlatformContext(ps);
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
        LineBatcher* grid_line_batcher = CreateLineBatcher(&ps->LineBatchers, "GridLineBatcher");
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
    Texture* diffuse_texture = CreateTexture(&ps->Textures,
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
    Texture* specular_texture = CreateTexture(&ps->Textures,
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
    Texture* emissive_texture = CreateTexture(&ps->Textures,
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
            .Vertices = kVertices.data(),
            .VertexCount = (u32)kVertices.size(),
            .Textures = {diffuse_texture, specular_texture, emissive_texture},
        };

        if (!CreateMesh(&ps->Meshes, "CubeMesh", options)) {
            return false;
        }
    }

    if (!CreateMesh(&ps->Meshes,
                    "LightMesh",
                    {
                        .Vertices = kVertices.data(),
                        .VertexCount = (u32)kVertices.size(),
                    })) {
        return false;
    }

    // Models.

    {
        path = ps->BasePath + "temp/models/backpack/backpack.obj";
		TempArena scratch = GetScratchArena();
		DEFER { ReleaseScratchArena(&scratch); };

        CreateModel(scratch.Arena, nullptr, "backpack", path.c_str());
    }

    // Shaders.

    {
        std::string vs_path = ps->BasePath + "assets/shaders/shader.vert";
        std::string fs_path = ps->BasePath + "assets/shaders/shader.frag";
        if (!CreateShader(&ps->Shaders.Registry,
                          "NormalShader",
                          vs_path.c_str(),
                          fs_path.c_str())) {
            return false;
        }
    }

    {
        std::string vs_path = ps->BasePath + "assets/shaders/light.vert";
        std::string fs_path = ps->BasePath + "assets/shaders/light.frag";
        if (!CreateShader(&ps->Shaders.Registry, "LightShader", vs_path.c_str(), fs_path.c_str())) {
            return false;
        }
    }

    {
        std::string vs_path = ps->BasePath + "assets/shaders/line_batcher.vert";
        std::string fs_path = ps->BasePath + "assets/shaders/line_batcher.frag";
        if (!CreateShader(&ps->Shaders.Registry,
                          "LineBatcherShader",
                          vs_path.c_str(),
                          fs_path.c_str())) {
            return false;
        }
    }

    // Prepare SSBO.
    glGenBuffers(1, &gs->SSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gs->SSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 2 * sizeof(float), NULL, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, gs->SSBO);

    return true;
}

bool GameUpdate(PlatformState* ps) {
    ImGuizmo::BeginFrame();
    ImGuizmo::Enable(true);
    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    GameState* gs = (GameState*)ps->GameState;
    ASSERT(gs);

    Update(ps, &gs->FreeCamera, ps->FrameDelta);

    if (MOUSE_PRESSED(ps, LEFT)) {
        if (gs->HoverEntityID != (u32)NONE) {
            gs->SelectedEntityID = gs->HoverEntityID;
        }
    }

    // Update the entities array.
    gs->EntityCount = 0;
    for (auto& position : kCubePositions) {
        Entity& entity = gs->Entities[gs->EntityCount++];
        entity.Type = EEntityType::Box;
        entity.Ptr = &position;
    }

    {
        Entity& entity = gs->Entities[gs->EntityCount++];
        entity.Type = EEntityType::DirectionalLight;
        entity.Ptr = &gs->DirectionalLight;
    }

    for (u32 i = 0; i < kNumPointLights; i++) {
        Entity& entity = gs->Entities[gs->EntityCount++];
        entity.Type = EEntityType::PointLight;
        entity.Ptr = &gs->PointLights[i];
    }

    {
        Entity& entity = gs->Entities[gs->EntityCount++];
        entity.Type = EEntityType::Spotlight;
        entity.Ptr = &gs->Spotlight;
    }

    if (ImGui::Begin("Kandinsky")) {
        ImGui::ColorEdit3("Clear Color", GetPtr(gs->ClearColor), ImGuiColorEditFlags_Float);

        ImGui::InputInt("Selected Entity",
                        (int*)&gs->SelectedEntityID,
                        1,
                        100,
                        ImGuiInputTextFlags_ReadOnly);

        ImGui::InputInt("Entity Index (hover)",
                        (int*)&gs->HoverEntityID,
                        1,
                        100,
                        ImGuiInputTextFlags_ReadOnly);

        if (ImGui::CollapsingHeader("Input", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::InputFloat2("Mouse",
                               GetPtr(ps->InputState.MousePosition),
                               "%.3f",
                               ImGuiInputTextFlags_ReadOnly);
            ImGui::InputFloat2("Mouse (GL)",
                               GetPtr(ps->InputState.MousePositionGL),
                               "%.3f",
                               ImGuiInputTextFlags_ReadOnly);
        }

        if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::InputFloat3("Position", GetPtr(gs->FreeCamera.Position));
        }

        ImGui::DragFloat("Shininess", &gs->Material.Shininess, 1.0f, 0.0f, 200);

        if (ImGui::TreeNodeEx("Lights",
                              ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
            if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                BuildImGui(&gs->DirectionalLight);
            }

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

        if (gs->SelectedEntityID != (u32)NONE) {
            Entity& entity = gs->Entities[gs->SelectedEntityID];

            if (entity.Type == EEntityType::PointLight) {
                PointLight* pl = (PointLight*)entity.Ptr;

                Debug::DrawSphere(ps, pl->Position, pl->MinRadius, 16, Color32::Black);
                Debug::DrawSphere(ps, pl->Position, pl->MaxRadius, 16, Color32::Grey);

                Mat4 model(1.0f);
                model = Translate(model, Vec3(pl->Position));
                if (ImGuizmo::Manipulate(GetPtr(gs->FreeCamera.View),
                                         GetPtr(gs->FreeCamera.Proj),
                                         ImGuizmo::TRANSLATE,
                                         ImGuizmo::WORLD,
                                         GetPtr(model))) {
                    pl->Position = model[3];
                }
#if SPOTLIGHT
                Spotlight& sl = gs->Spotlight;

                Vec3 direction = GetDirection(sl);
                Debug::DrawCone(ps,
                                sl.Position,
                                direction,
                                sl.MaxCutoffDistance,
                                ToRadians(sl.OuterRadiusDeg),
                                16,
                                Color32::Orange);

                Mat4 posmat(1.0f);
                posmat = Translate(posmat, Vec3(sl.Position));
                if (ImGuizmo::Manipulate(GetPtr(gs->FreeCamera.View),
                                         GetPtr(gs->FreeCamera.Proj),
                                         ImGuizmo::TRANSLATE,
                                         ImGuizmo::WORLD,
                                         GetPtr(posmat))) {
                    sl.Position = posmat[3];
                    Recalculate(&sl);
                }
#endif  // SPOTLIGHT
            }
        }

        ImGui::End();
    }

    return true;
}

bool GameRender(PlatformState* ps) {
    using namespace kdk;

    GameState* gs = (GameState*)ps->GameState;
    ASSERT(gs);

    LineBatcher* grid_line_batcher = FindLineBatcher(&ps->LineBatchers, "GridLineBatcher");
    ASSERT(grid_line_batcher);

    Mesh* cube_mesh = FindMesh(&ps->Meshes, "CubeMesh");
    ASSERT(cube_mesh);
    Mesh* light_mesh = FindMesh(&ps->Meshes, "LightMesh");
    ASSERT(light_mesh);

    Shader* normal_shader = FindShader(&ps->Shaders.Registry, "NormalShader");
    ASSERT(normal_shader);
    Shader* light_shader = FindShader(&ps->Shaders.Registry, "LightShader");
    ASSERT(light_shader);
    Shader* line_batcher_shader = FindShader(&ps->Shaders.Registry, "LineBatcherShader");
    ASSERT(line_batcher_shader);

    // Texture* diffuse_texture = FindTexture(&ps->Textures, "DiffuseTexture");
    // ASSERT(diffuse_texture);
    // Texture* specular_texture = FindTexture(&ps->Textures, "SpecularTexture");
    // ASSERT(specular_texture);
    // Texture* emissive_texture = FindTexture(&ps->Textures, "EmissionTexture");
    // ASSERT(emissive_texture);

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
    rs.Spotlight.InnerRadiusCos = Cos(ToRadians(gs->Spotlight.InnerRadiusDeg));
    rs.Spotlight.OuterRadiusCos = Cos(ToRadians(gs->Spotlight.OuterRadiusDeg));

    glViewport(0, 0, ps->Window.Width, ps->Window.Height);

    glEnable(GL_DEPTH_TEST);

    glClearColor(gs->ClearColor.r, gs->ClearColor.g, gs->ClearColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // "Reset" the SSBO.

    {
        float values[2] = {
            std::numeric_limits<float>::max(),
            -1.0f,
        };
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(values), values);
    }

    // Render plane.
    {
        Use(*line_batcher_shader);
        SetMat4(*line_batcher_shader, "uViewProj", GetPtr(gs->FreeCamera.ViewProj));
        Draw(*grid_line_batcher, *line_batcher_shader);
    }

    // Render entities.

    const Mat4& view = gs->FreeCamera.View;
    for (u32 entity_index = 0; entity_index < gs->EntityCount; entity_index++) {
        Entity& entity = gs->Entities[entity_index];

        // Render cubes.
        if (entity.Type == EEntityType::Box) {
            Use(*normal_shader);

            SetFloat(*normal_shader, "uMaterial.Shininess", gs->Material.Shininess);
            const auto& position = *(Vec3*)(entity.Ptr);

            // TODO(cdc): These should be handled on update and just rendered here.
            Mat4 model = Mat4(1.0f);
            model = Translate(model, position);
            model = Rotate(model, ToRadians(rs.Seconds * 25), Vec3(1.0f, 0.0f, 0.0f));

            Mat4 view_model = view * model;
            Mat4 normal_matrix = Transpose(Inverse(view_model));

            SetMat4(*normal_shader, "uViewModel", GetPtr(view_model));
            SetMat4(*normal_shader, "uNormalMatrix", GetPtr(normal_matrix));
            SetVec2(*normal_shader, "uMouseCoords", ps->InputState.MousePositionGL);
            SetFloat(*normal_shader, "uObjectID", (float)entity_index);

            DrawMesh(*cube_mesh, *normal_shader, rs);

            continue;
        }

        // Render Lights.
        if (entity.Type == EEntityType::PointLight) {
            Use(*light_shader);

            SetVec2(*light_shader, "uMouseCoords", ps->InputState.MousePositionGL);
            SetFloat(*light_shader, "uObjectID", (float)entity_index);

            PointLight* pl = (PointLight*)(entity.Ptr);
            Draw(*pl, *light_shader, *light_mesh, rs);
        }
    }

    kdk::Debug::Render(ps, *line_batcher_shader, gs->FreeCamera.ViewProj);

    // Read the SSBO value.
    {
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        float values[2] = {};
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, gs->SSBO);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(values), values);

        gs->HoverEntityID = (u32)values[1];
    }

    return true;
}

}  // namespace kdk

#ifdef __cplusplus
}
#endif

#include <learn_opengl/learn_opengl.h>

#include <kandinsky/debug.h>
#include <kandinsky/defines.h>
#include <kandinsky/imgui.h>
#include <kandinsky/math.h>
#include <kandinsky/platform.h>
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
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        SDL_Log("GLEW Init error: %s\n", glewGetErrorString(glewError));
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
    GameState* game_state = (GameState*)ArenaPush(&ps->Memory.PermanentArena, sizeof(GameState));
    *game_state = {};
    ps->GameState = game_state;

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

    // Meshes.
    if (!CreateMesh(ps,
                    &ps->Meshes,
                    "CubeMesh",
                    {
                        .VerticesCount = (u32)kVertices.size(),
                        .Vertices = kVertices.data(),
                    })) {
        return false;
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

    // Textures.

    {
        std::string path = ps->BasePath + "assets/textures/container2.png";
        if (!CreateTexture(ps, &ps->Textures, "DiffuseTexture", path.c_str())) {
            SDL_Log("ERROR: Loading diffuse texture");
            return false;
        }
    }

    {
        std::string path = ps->BasePath + "assets/textures/container2_specular.png";
        if (!CreateTexture(ps, &ps->Textures, "SpecularTexture", path.c_str())) {
            SDL_Log("ERROR: Loading specular texture");
            return false;
        }
    }

    {
        std::string path = ps->BasePath + "assets/textures/matrix.jpg";
        if (!CreateTexture(ps,
                           &ps->Textures,
                           "EmissionTexture",
                           path.c_str(),
                           {
                               .WrapT = GL_MIRRORED_REPEAT,
                           })) {
            SDL_Log("ERROR: Loading emission texture");
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

        if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Type:");
            ImGui::SameLine();

            for (u8 i = 0; i < (u8)ELightType::COUNT; i++) {
                ELightType light_type = (ELightType)i;

                bool active = gs->Light.Type == light_type;
                if (ImGui::RadioButton(ToString(light_type), active)) {
                    gs->Light.Type = light_type;
                }

                if (i < (u8)ELightType::COUNT - 1) {
                    ImGui::SameLine();
                }
            }

            switch (gs->Light.Type) {
                case ELightType::Point: {
                    ImGui::InputFloat3("Position", GetPtr(gs->Light.Position));

                    ImGui::Text("Attenuation");
                    ImGui::DragFloat("Constant",
                                     &gs->Light.Attenuation.Constant,
                                     0.01f,
                                     0.0f,
                                     4.0f);
                    ImGui::DragFloat("Linear", &gs->Light.Attenuation.Linear, 0.01f, 0.0f, 1.0f);
                    ImGui::DragFloat("Quadratic",
                                     &gs->Light.Attenuation.Quadratic,
                                     0.001f,
                                     0.0f,
                                     1.0f);

                    ImGui::DragFloat("MinRadius", &gs->Light.MinRadius, 0.01f, 0.0f, 1.0f);
                    ImGui::DragFloat("MaxRadius", &gs->Light.MaxRadius, 0.01f, 0.0f, 10.0f);
                    break;
                }
                case ELightType::Directional: {
                    ImGui::InputFloat3("Direction", GetPtr(gs->Light.Position));
                    break;
                }
                case ELightType::Spotlight: {
                    break;
                }

                case ELightType::COUNT: break;
            }
        }

        switch (gs->Light.Type) {
            case ELightType::Point: {
                Debug::DrawSphere(ps, gs->Light.Position, gs->Light.MinRadius, 16, Color32::Black);
                Debug::DrawSphere(ps, gs->Light.Position, gs->Light.MaxRadius, 16, Color32::Grey);
                break;
            }
            case ELightType::Directional: {
                break;
            }
            case ELightType::Spotlight: {
                const auto& light_pos = gs->Light.Position;
                Vec3 spotlight_target = Vec3(0);
                Debug::DrawCone(ps,
                                light_pos,
                                spotlight_target - light_pos,
                                glm::distance(light_pos, spotlight_target),
                                glm::radians(12.5f),
                                8,
                                Color32::Orange,
                                3.0f);
                break;
            }

            case ELightType::COUNT: break;
        }

        ImGui::End();
    }

    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(gs->Light.Position));
    if (ImGuizmo::Manipulate(GetPtr(gs->FreeCamera.View),
                             GetPtr(gs->FreeCamera.Proj),
                             ImGuizmo::TRANSLATE,
                             ImGuizmo::WORLD,
                             GetPtr(model))) {
        gs->Light.Position = model[3];
    }

    /* Debug::DrawArrow(ps, Vec3(1), Vec3(1, 1, -1), Color32::SkyBlue, 0.05f, 3.0f); */

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

    Texture* diffuse_texture = FindTexture(&ps->Textures, "DiffuseTexture");
    assert(diffuse_texture);
    Texture* specular_texture = FindTexture(&ps->Textures, "SpecularTexture");
    assert(specular_texture);
    Texture* emission_texture = FindTexture(&ps->Textures, "EmissionTexture");
    assert(emission_texture);

    /* float seconds = 0.5f * static_cast<float>(SDL_GetTicks()) / 1000.0f; */
    float seconds = 0;

    auto light_position = Vec4(gs->Light.Position, 1.0f);
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
        const Mat4& proj = gs->FreeCamera.Proj;

        Use(*normal_shader);
        Bind(*cube_mesh);

        // Set the material indices.
        SetI32(*normal_shader, "uMaterial.Diffuse", 0);
        SetI32(*normal_shader, "uMaterial.Specular", 1);
        SetI32(*normal_shader, "uMaterial.Emission", 2);
        Bind(*diffuse_texture, GL_TEXTURE0);
        Bind(*specular_texture, GL_TEXTURE1);
        glBindTexture(GL_TEXTURE2, NULL);
        /* Bind(ps, *emission_texture, GL_TEXTURE2); */

        SetVec3(*normal_shader, "uMaterial.Specular", Vec3(0.5f, 0.5f, 0.5f));
        SetFloat(*normal_shader, "uMaterial.Shininess", 32.0f);

        /* Vec4 view_light_position = view * Vec4(light_position, 2.0f); */
        Vec4 view_light_position = view * light_position;
        SetVec3(*normal_shader, "uLight.Ambient", Vec3(0.2f, 0.2f, 0.2f));
        SetVec3(*normal_shader, "uLight.Diffuse", Vec3(0.5f, 0.5f, 0.5f));
        SetVec3(*normal_shader, "uLight.Specular", Vec3(1.0f, 1.0f, 1.0f));
        SetAttenuation(*normal_shader, gs->Light);

        Vec4 view_spotlight_target = view * Vec4(0.0f, 0.0f, 0.0f, 1.0f);
        Vec3 view_spotlight_direction = view_spotlight_target - view_light_position;
        SetVec3(*normal_shader, "uLight.Spotlight.Direction", view_spotlight_direction);
        SetFloat(*normal_shader, "uLight.Spotlight.InnerRadiusCos", glm::cos(glm::radians(12.5f)));
        SetFloat(*normal_shader, "uLight.Spotlight.OuterRadiusCos", glm::cos(glm::radians(15.0f)));

        SetFloat(*normal_shader, "uTime", seconds);

        SetMat4(*normal_shader, "uProj", GetPtr(proj));

        // We send this magical number just at the end... otherwise it messes up with the math.
        switch (gs->Light.Type) {
            case ELightType::Point: view_light_position.w = 1.0f; break;
            case ELightType::Directional: view_light_position.w = 0.0f; break;
            case ELightType::Spotlight: view_light_position.w = 2.0f; break;
            case ELightType::COUNT: break;
        }
        SetVec4(*normal_shader, "uLight.PosDir", view_light_position);

        for (const auto& position : kCubePositions) {
            Mat4 model = Mat4(1.0f);
            model = glm::translate(model, position);
            model = glm::rotate(model, glm::radians(seconds * 25), Vec3(1.0f, 0.0f, 0.0f));

            Mat4 view_model = view * model;
            Mat4 normal_matrix = glm::transpose(glm::inverse(view_model));

            SetMat4(*normal_shader, "uViewModel", GetPtr(view_model));
            SetMat4(*normal_shader, "uNormalMatrix", GetPtr(normal_matrix));

            // glDrawArrays(GL_TRIANGLES, 0, 3);
            /* glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); */
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }

    // Render light.
    {
        RenderState_Light light_rs{
            .Light = &gs->Light,
            .Shader = light_shader,
            .Mesh = light_mesh,
            .ViewProj = &gs->FreeCamera.ViewProj,
        };
        RenderLight(&light_rs);
    }

    kdk::Debug::Render(ps, *line_batcher_shader, gs->FreeCamera.ViewProj);

    return true;
}

}  // namespace kdk

#ifdef __cplusplus
}
#endif

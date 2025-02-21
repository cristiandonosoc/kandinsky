#include <kandinsky/apps/learn_opengl.h>

#include <kandinsky/debug.h>
#include <kandinsky/defines.h>
#include <kandinsky/imgui.h>
#include <kandinsky/math.h>
#include <kandinsky/platform.h>
#include <kandinsky/utils/defer.h>
#include <kandinsky/window.h>

#include <SDL3/SDL_mouse.h>

#include <imgui.h>

#include <string>

#ifdef __cplusplus
extern "C" {
#endif

namespace kdk {

// clang-format off
float kVertices[] = {
    // positions          // normals           // texture coords
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
};

glm::vec3 kCubePositions[] = {glm::vec3(0.0f, 0.0f, 0.0f),
                              glm::vec3(2.0f, 5.0f, -15.0f),
                              glm::vec3(-1.5f, -2.2f, -2.5f),
                              glm::vec3(-3.8f, -2.0f, -12.3f),
                              glm::vec3(2.4f, -0.4f, -3.5f),
                              glm::vec3(-1.7f, 3.0f, -7.5f),
                              glm::vec3(1.3f, -2.0f, -2.5f),
                              glm::vec3(1.5f, 2.0f, -2.5f),
                              glm::vec3(1.5f, 0.2f, -1.5f),
                              glm::vec3(-1.3f, 1.0f, -1.5f)};
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
            std::array<glm::vec3, 4> points{
                // X-axis.
                glm::vec3(i, 0, -kMeters),
                glm::vec3(i, 0, kMeters),
                // Z-Axis.
                glm::vec3(-kMeters, 0, i),
                glm::vec3( kMeters, 0, i),
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
                        .Vertices = {kVertices, std::size(kVertices)},
                        .AttribPointers = {3, 3, 2},
                    })) {
        return false;
    }

    if (!CreateMesh(ps,
                    &ps->Meshes,
                    "LightMesh",
                    {
                        .Vertices = {kVertices, std::size(kVertices)},
                        .AttribPointers = {3},
                        .Stride = 8 * sizeof(float),
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
    GameState* gs = (GameState*)ps->GameState;
    assert(gs);

    Update(ps, &gs->FreeCamera, ps->FrameDelta);

    if (ImGui::Begin("Kandinsky")) {
        if (ImGui::CollapsingHeader("Light")) {
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
                    ImGui::InputFloat3("Position", glm::value_ptr(gs->Light.Position));
                    Debug::DrawSphere(ps, gs->Light.Position, 2.0f, 16, Color32::Blue, 2.0f);
                    break;
                }
                case ELightType::Directional: {
                    ImGui::InputFloat3("Direction", glm::value_ptr(gs->Light.Position));
                    break;
                }
                case ELightType::Spotlight: {
                    const auto& light_pos = gs->Light.Position;
                    glm::vec3 spotlight_target = glm::vec3(0);
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

                case ELightType::COUNT:
                    break;
            }
        }

        ImGui::End();
    }

    /* Debug::DrawArrow(ps, glm::vec3(1), glm::vec3(1, 1, -1), Color32::SkyBlue, 0.05f, 3.0f); */

    return true;
}

bool GameRender(PlatformState* ps) {
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

    auto light_position = glm::vec4(gs->Light.Position, 0.0f);
    switch (gs->Light.Type) {
        case ELightType::Point:
            light_position.w = 1.0f;
            break;
        case ELightType::Directional:
            light_position.w = 0.0f;
            break;
        case ELightType::Spotlight:
            light_position.w = 2.0f;
            break;
        case ELightType::COUNT:
            break;
    }

    glViewport(0, 0, ps->Window.Width, ps->Window.Height);

    glEnable(GL_DEPTH_TEST);

    /* glClearColor(0.3f, 0.3f, 0.3f, 1.0f); */
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 view = GetViewMatrix(gs->FreeCamera);
    float aspect_ratio = (float)(ps->Window.Width) / (float)(ps->Window.Height);

    glm::mat4 proj = glm::mat4(1.0f);
    proj = glm::perspective(glm::radians(45.0f), aspect_ratio, 0.1f, 100.0f);
    glm::mat4 view_proj = proj * view;

    kdk::Debug::Render(ps, *line_batcher_shader, view_proj);

    // Render plane.
    {
        Use(ps, *line_batcher_shader);
        SetMat4(ps, *line_batcher_shader, "uViewProj", glm::value_ptr(view_proj));
        Draw(ps, *line_batcher_shader, *grid_line_batcher);
    }

    // Render cubes.
    {
        Use(ps, *normal_shader);
        Bind(ps, *cube_mesh);

        // Set the material indices.
        SetI32(ps, *normal_shader, "uMaterial.Diffuse", 0);
        SetI32(ps, *normal_shader, "uMaterial.Specular", 1);
        SetI32(ps, *normal_shader, "uMaterial.Emission", 2);
        Bind(ps, *diffuse_texture, GL_TEXTURE0);
        Bind(ps, *specular_texture, GL_TEXTURE1);
        glBindTexture(GL_TEXTURE2, NULL);
        /* Bind(ps, *emission_texture, GL_TEXTURE2); */

        SetVec3(ps, *normal_shader, "uMaterial.Specular", glm::vec3(0.5f, 0.5f, 0.5f));
        SetFloat(ps, *normal_shader, "uMaterial.Shininess", 32.0f);

        /* glm::vec4 view_light_position = view * glm::vec4(light_position, 2.0f); */
        glm::vec4 view_light_position = view * light_position;
        SetVec4(ps, *normal_shader, "uLight.PosDir", view_light_position);
        SetVec3(ps, *normal_shader, "uLight.Ambient", glm::vec3(0.2f, 0.2f, 0.2f));
        SetVec3(ps, *normal_shader, "uLight.Diffuse", glm::vec3(0.5f, 0.5f, 0.5f));
        SetVec3(ps, *normal_shader, "uLight.Specular", glm::vec3(1.0f, 1.0f, 1.0f));
        SetFloat(ps, *normal_shader, "uLight.Attenuation.Constant", 1.0f);
        SetFloat(ps, *normal_shader, "uLight.Attenuation.Linear", 0.09f);
        SetFloat(ps, *normal_shader, "uLight.Attenuation.Quadratic", 0.032f);

        glm::vec4 spotlight_target = view * glm::vec4(0);
        glm::vec3 spotlight_direction = spotlight_target - view_light_position;
        SetVec3(ps, *normal_shader, "uLight.Spotlight.Direction", spotlight_direction);
        SetFloat(ps, *normal_shader, "uLight.Spotlight.Cutoff", glm::cos(glm::radians(12.5f)));

        SetFloat(ps, *normal_shader, "uTime", seconds);

        SetMat4(ps, *normal_shader, "uProj", glm::value_ptr(proj));

        for (const auto& position : kCubePositions) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, position);
            model = glm::rotate(model, glm::radians(seconds * 25), glm::vec3(1.0f, 0.0f, 0.0f));

            glm::mat4 view_model = view * model;
            glm::mat4 normal_matrix = glm::transpose(glm::inverse(view_model));

            SetMat4(ps, *normal_shader, "uViewModel", glm::value_ptr(view_model));
            SetMat4(ps, *normal_shader, "uNormalMatrix", glm::value_ptr(normal_matrix));

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
            .ViewProj = &view_proj,
        };
        RenderLight(ps, &light_rs);
    }

    return true;
}

}  // namespace kdk

#ifdef __cplusplus
}
#endif

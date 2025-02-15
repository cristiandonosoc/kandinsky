#include <SDL3/SDL_mouse.h>
#include <kandinsky/defines.h>
#include <kandinsky/imgui.h>
#include <kandinsky/utils/defer.h>
#include <kandinsky/window.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>

static bool gDone = false;

static constexpr int kWidth = 1440;
static constexpr int kHeight = 1080;

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

glm::vec3 kCubePositions[] = {
    glm::vec3( 0.0f,  0.0f,  0.0f),
    glm::vec3( 2.0f,  5.0f, -15.0f),
    glm::vec3(-1.5f, -2.2f, -2.5f),
    glm::vec3(-3.8f, -2.0f, -12.3f),
    glm::vec3( 2.4f, -0.4f, -3.5f),
    glm::vec3(-1.7f,  3.0f, -7.5f),
    glm::vec3( 1.3f, -2.0f, -2.5f),
    glm::vec3( 1.5f,  2.0f, -2.5f),
    glm::vec3( 1.5f,  0.2f, -1.5f),
    glm::vec3(-1.3f,  1.0f, -1.5f)
};

glm::vec3 gLightPosition = glm::vec3(1.2f, 1.0f, 2.0f);
// clang-format on

glm::vec3 gUp = glm::vec3(0.0f, 1.0f, 0.0f);

std::string kBasePath;

kdk::Shader gNormalShader = {};
kdk::Shader gLightShader = {};
kdk::Shader gLineBatcherShader = {};

kdk::Mesh gCubeMesh = {};
kdk::Mesh gLightMesh = {};

kdk::Texture gTexture1 = {};
kdk::Texture gTexture2 = {};

kdk::Texture gDiffuseTexture = {};
kdk::Texture gSpecularTexture = {};
kdk::Texture gEmissionTexture = {};

u64 gLastFrameTicks = 0;
float gFrameDelta = 0;

glm::vec2 gLastMousePos = glm::vec2(kWidth / 2, kHeight / 2);

kdk::Camera gFreeCamera = {
    .Position = glm::vec3(-4.0f, 1.0f, 0.0f),
};

kdk::LineBatcher gLineBatcher;
kdk::LineBatcher gAxisBatcher;

bool InitRender() {
    // Line Batchers.

    {
        gLineBatcher = kdk::CreateLineBatcher();
        if (!IsValid(gLineBatcher)) {
            SDL_Log("ERROR: creating line batcher");
            return false;
        }

        // Add a plane.
        glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
        constexpr i32 kMeters = 100;
        for (i32 i = -kMeters; i <= kMeters; i++) {
            if (i == 0) {
                continue;
            }

            // clang-format off
            std::array<kdk::LineBatcherPoint, 4> points{
                // X-axis.
                kdk::LineBatcherPoint{.Position = glm::vec3(i, 0, -kMeters), .Color = color},
                kdk::LineBatcherPoint{.Position = glm::vec3(i, 0, kMeters), .Color = color},
                // Z-Axis.
                kdk::LineBatcherPoint{.Position = glm::vec3(-kMeters, 0, i), .Color = color},
                kdk::LineBatcherPoint{.Position = glm::vec3( kMeters, 0, i), .Color = color},
            };
            // clang-format on

            AddPoints(&gLineBatcher, points);
        };

        Buffer(gLineBatcher);

        gAxisBatcher = kdk::CreateLineBatcher();
        if (!IsValid(gAxisBatcher)) {
            SDL_Log("ERROR: Creating axis batcher");
            return false;
        }

        // clang-format off
        std::array<kdk::LineBatcherPoint, 6> axis{
            kdk::LineBatcherPoint{.Position = {-kMeters,        0,        0}, .Color = {1, 0, 0}},
            kdk::LineBatcherPoint{.Position = { kMeters,        0,        0}, .Color = {1, 0, 0}},
            kdk::LineBatcherPoint{.Position = {       0, -kMeters,        0}, .Color = {0, 1, 0}},
            kdk::LineBatcherPoint{.Position = {       0,  kMeters,        0}, .Color = {0, 1, 0}},
            kdk::LineBatcherPoint{.Position = {		  0,        0, -kMeters}, .Color = {0, 0, 1}},
            kdk::LineBatcherPoint{.Position = {		  0,        0,  kMeters}, .Color = {0, 0, 1}},
        };
        // clang-format on
        AddPoints(&gAxisBatcher, axis);
        Buffer(gAxisBatcher);
    }

    // Meshes.
    gCubeMesh = kdk::CreateMesh("NormalMesh",
                                {
                                    .Vertices = {kVertices, std::size(kVertices)},
                                    .AttribPointers = {3, 3, 2},
                                });
    if (!IsValid(gCubeMesh)) {
        return false;
    }

    gLightMesh = kdk::CreateMesh("LightMesh",
                                 {
                                     .Vertices = {kVertices, std::size(kVertices)},
                                     .AttribPointers = {3},
                                     .Stride = 8 * sizeof(float),
                                 });
    if (!IsValid(gLightMesh)) {
        return false;
    }

    // Shaders.

    {
        std::string vs_path = kBasePath + "assets/shaders/shader.vert";
        std::string fs_path = kBasePath + "assets/shaders/shader.frag";
        auto shader = kdk::CreateShader("NormalShader", vs_path.c_str(), fs_path.c_str());
        if (!IsValid(shader)) {
            return false;
        }

        gNormalShader = shader;
    }

    {
        std::string vs_path = kBasePath + "assets/shaders/light.vert";
        std::string fs_path = kBasePath + "assets/shaders/light.frag";
        auto shader = kdk::CreateShader("LightShader", vs_path.c_str(), fs_path.c_str());
        if (!IsValid(shader)) {
            return false;
        }

        gLightShader = shader;
    }

    {
        std::string vs_path = kBasePath + "assets/shaders/line_batcher.vert";
        std::string fs_path = kBasePath + "assets/shaders/line_batcher.frag";
        auto shader = kdk::CreateShader("LineBatcherShader", vs_path.c_str(), fs_path.c_str());
        if (!IsValid(shader)) {
            return false;
        }

        gLineBatcherShader = shader;
    }

    // Textures.

    {
        std::string path = kBasePath + "assets/textures/wall.jpg";
        gTexture1 = kdk::LoadTexture(path.c_str());
        if (!IsValid(gTexture1)) {
            SDL_Log("ERROR: Loading texture1");
            return false;
        }
    }

    {
        std::string path = kBasePath + "assets/textures/awesomeface.png";
        gTexture2 = kdk::LoadTexture(path.c_str(),
                                     {
                                         .FlipVertically = true,
                                     });
        if (!IsValid(gTexture2)) {
            SDL_Log("ERROR: Loading texture2");
            return false;
        }
    }

    {
        std::string path = kBasePath + "assets/textures/container2.png";
        gDiffuseTexture = kdk::LoadTexture(path.c_str());
        if (!IsValid(gDiffuseTexture)) {
            SDL_Log("ERROR: Loading diffuse texture");
            return false;
        }
    }

    {
        std::string path = kBasePath + "assets/textures/container2_specular.png";
        gSpecularTexture = kdk::LoadTexture(path.c_str());
        if (!IsValid(gSpecularTexture)) {
            SDL_Log("ERROR: Loading specular texture");
            return false;
        }
    }

    {
        std::string path = kBasePath + "assets/textures/matrix.jpg";
        gEmissionTexture = kdk::LoadTexture(path.c_str(),
                                            {
                                                .WrapT = GL_MIRRORED_REPEAT,
                                            });
        if (!IsValid(gEmissionTexture)) {
            SDL_Log("ERROR: Loading emission texture");
            return false;
        }
    }

    return true;
}

void Render() {
    using namespace kdk;

    glViewport(0, 0, kWidth, kHeight);

    // float seconds = static_cast<float>(SDL_GetTicks()) / 1000.0f;
    float seconds = 0;

    glEnable(GL_DEPTH_TEST);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 view = GetViewMatrix(gFreeCamera);
    float aspect_ratio = static_cast<float>(kWidth) / static_cast<float>(kHeight);

    /* constexpr float kLightRadius = 3.0f; */
    /* float light_rot_speed = 2 * seconds; */
    /* gLightPosition = */
    /*     glm::vec3(kLightRadius * cos(light_rot_speed), 1.0f, kLightRadius *
     * sin(light_rot_speed)); */

    glm::mat4 proj = glm::mat4(1.0f);
    proj = glm::perspective(glm::radians(45.0f), aspect_ratio, 0.1f, 100.0f);

    glm::mat4 view_proj = proj * view;

    // Render plane.
    {
        Use(gLineBatcherShader);

        SetMat4(gLineBatcherShader, "uViewProj", glm::value_ptr(view_proj));

        glLineWidth(1.0f);
        Draw(gLineBatcher);

        glLineWidth(3.0f);
        Draw(gAxisBatcher);

        glLineWidth(1.0f);
    }

    // Render cubes.
    {
        Use(gNormalShader);
        Bind(gCubeMesh);

        // Set the material indices.
        SetI32(gNormalShader, "uMaterial.Diffuse", 0);
        SetI32(gNormalShader, "uMaterial.Specular", 1);
        SetI32(gNormalShader, "uMaterial.Emission", 2);
        Bind(gDiffuseTexture, GL_TEXTURE0);
        Bind(gSpecularTexture, GL_TEXTURE1);
        Bind(gEmissionTexture, GL_TEXTURE2);

        SetVec3(gNormalShader, "uMaterial.Specular", glm::vec3(0.5f, 0.5f, 0.5f));
        SetFloat(gNormalShader, "uMaterial.Shininess", 32.0f);

        glm::vec4 view_light_position = view * glm::vec4(gLightPosition, 1.0f);
        SetVec4(gNormalShader, "uLight.PosDir", view_light_position);
        SetVec3(gNormalShader, "uLight.Ambient", glm::vec3(0.2f, 0.2f, 0.2f));
        SetVec3(gNormalShader, "uLight.Diffuse", glm::vec3(0.5f, 0.5f, 0.5f));
        SetVec3(gNormalShader, "uLight.Specular", glm::vec3(1.0f, 1.0f, 1.0f));
        SetFloat(gNormalShader, "uLight.Attenuation.Constant", 1.0f);
        SetFloat(gNormalShader, "uLight.Attenuation.Linear", 5 * 0.09f);
        SetFloat(gNormalShader, "uLight.Attenuation.Quadratic", 5 * 0.032f);

        glm::vec4 spotlight_target = view * glm::vec4(0);
        glm::vec3 spotlight_direction = spotlight_target - view_light_position;
        SetVec3(gNormalShader, "uLight.Spotlight.Direction", spotlight_direction);
        SetFloat(gNormalShader, "uLight.Spotlight.Cutoff", glm::cos(glm::radians(12.5f)));

        SetFloat(gNormalShader, "uTime", seconds);

        SetMat4(gNormalShader, "uProj", glm::value_ptr(proj));

        for (const auto& position : kCubePositions) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, position);
            model = glm::rotate(model, glm::radians(seconds * 25), glm::vec3(1.0f, 0.0f, 0.0f));

            glm::mat4 view_model = view * model;
            glm::mat4 normal_matrix = glm::transpose(glm::inverse(view_model));

            SetMat4(gNormalShader, "uViewModel", glm::value_ptr(view_model));
            SetMat4(gNormalShader, "uNormalMatrix", glm::value_ptr(normal_matrix));

            // glDrawArrays(GL_TRIANGLES, 0, 3);
            /* glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); */
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }

    // Render light.
    {
        Use(gLightShader);
        Bind(gLightMesh);

        SetMat4(gLightShader, "uViewProj", glm::value_ptr(view_proj));

        glm::mat4 model(1.0f);
        model = glm::translate(model, gLightPosition);
        model = glm::scale(model, glm::vec3(0.2f));
        SetMat4(gLightShader, "uModel", glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    RenderImgui();
}

bool Update() {
    using namespace kdk;

    BeginImguiFrame();

    if (!PollWindowEvents()) {
        return false;
    }

    static bool gShowDemoWindow = true;
    ImGui::ShowDemoWindow(&gShowDemoWindow);

    Update(&gFreeCamera, gFrameDelta);

    return true;
}

int main() {
    using namespace kdk;

    if (!InitWindow("kandinsky", kWidth, kHeight)) {
        return -1;
    }
    DEFER { ShutdownWindow(); };

    if (!InitImgui()) {
        return -1;
    }
    DEFER { ShutdownImgui(); };

    kBasePath = SDL_GetCurrentDirectory();

    if (!InitRender()) {
        return -1;
    }

    while (!gDone) {
        u64 current_frame_ticks = SDL_GetTicksNS();
        if (gLastFrameTicks != 0) {
            u64 delta_ticks = current_frame_ticks - gLastFrameTicks;
            // Transform to seconds.
            gFrameDelta = static_cast<float>(delta_ticks) / 1'000'000'000.0f;
        }
        gLastFrameTicks = current_frame_ticks;

        if (!Update()) {
            break;
        }

        Render();

        if (!SDL_GL_SwapWindow(gWindow.SDLWindow)) {
            SDL_Log("ERROR: Could not swap buffer: %s\n", SDL_GetError());
            return -1;
        }
    }

    return 0;
}

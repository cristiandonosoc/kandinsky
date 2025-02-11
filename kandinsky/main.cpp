#include <SDL3/SDL_mouse.h>
#include <kandinsky/defines.h>
#include <kandinsky/utils/defer.h>
#include <kandinsky/window.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <string>

static bool gDone = false;

static constexpr int kWidth = 1024;
static constexpr int kHeight = 720;

// clang-format off
/*
float kVertices[] = {
	// Positions          // UVs
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
};
*/

float kVertices[] = {
	// Position           // Normal
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
};

/* u32 kIndices[] = { */
/* 	0, 1, 3, */
/* 	1, 2, 3, */
/* }; */

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

kdk::Mesh gCubeMesh = {};
kdk::Mesh gLightMesh = {};

kdk::Texture gTexture1 = {};
kdk::Texture gTexture2 = {};

u64 gLastFrameTicks = 0;
float gFrameDelta = 0;

glm::vec2 gLastMousePos = glm::vec2(kWidth / 2, kHeight / 2);

kdk::Camera gFreeCamera = {
    .Position = glm::vec3(-4.0f, 0.0f, 0.0f),
};

bool InitRender() {
    gCubeMesh = kdk::CreateMesh("NormalMesh",
                                {
                                    .Vertices = {kVertices, std::size(kVertices)},
                                    .AttribPointers = {3, 3},
                                });

    gLightMesh = kdk::CreateMesh("LightMesh",
                                 {
                                     .Vertices = {kVertices, std::size(kVertices)},
                                     .AttribPointers = {3},
                                     .Stride = 6 * sizeof(float),
                                 });

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

    std::string path = kBasePath + "assets/textures/wall.jpg";
    gTexture1 = kdk::LoadTexture(path.c_str());
    if (!IsValid(gTexture1)) {
        SDL_Log("ERROR: Loading texture1");
        return false;
    }

    path = kBasePath + "assets/textures/awesomeface.png";
    gTexture2 = kdk::LoadTexture(path.c_str(),
                                 {
                                     .FlipVertically = true,
                                 });
    if (!IsValid(gTexture2)) {
        SDL_Log("ERROR: Loading texture2");
        return false;
    }

    return true;
}

void Render() {
    using namespace kdk;

    glEnable(GL_DEPTH_TEST);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float seconds = static_cast<float>(SDL_GetTicks()) / 1000.0f;

    glm::mat4 view = GetViewMatrix(gFreeCamera);
    float aspect_ratio = static_cast<float>(kWidth) / static_cast<float>(kHeight);

    glm::mat4 proj = glm::mat4(1.0f);
    proj = glm::perspective(glm::radians(45.0f), aspect_ratio, 0.1f, 100.0f);

    // Render cubes.
    {
        Use(gNormalShader);
        Bind(gCubeMesh);
        /* SetI32(gNormalShader, "uTex1", 0); */
        /* SetI32(gNormalShader, "uTex2", 1); */
        glm::vec3 object_color(1.0f, 0.5f, 0.31f);
        SetVec3(gNormalShader, "uObjectColor", glm::value_ptr(object_color));

		//SDL_Log("LIGHT POS: (%f, %f, %f)\n", gLightPosition[0], gLightPosition[1], gLightPosition[2]);
        SetVec3(gNormalShader, "uLightPosition", glm::value_ptr(gLightPosition));

        glm::vec3 light_color(1.0f, 1.0f, 1.0f);
        SetVec3(gNormalShader, "uLightColor", glm::value_ptr(light_color));

		SetVec3(gNormalShader, "uCameraPosition", glm::value_ptr(gFreeCamera.Position));

        Bind(gTexture1, GL_TEXTURE0);
        Bind(gTexture2, GL_TEXTURE1);

        SetMat4(gNormalShader, "uView", glm::value_ptr(view));
        SetMat4(gNormalShader, "uProj", glm::value_ptr(proj));

        for (const auto& position : kCubePositions) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, position);
            model = glm::rotate(model, glm::radians(seconds * 25), glm::vec3(1.0f, 0.0f, 0.0f));

            SetMat4(gNormalShader, "uModel", glm::value_ptr(model));

            // glDrawArrays(GL_TRIANGLES, 0, 3);
            /* glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); */
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }

    // Render light.
    {
        Use(gLightShader);
        Bind(gLightMesh);

        /* glm::vec3 object_color(1.0f, 0.5f, 0.31f); */
        /* SetVec3(gLightShader, "uObjectColor", glm::value_ptr(object_color)); */

        /* glm::vec3 light_color(1.0f, 1.0f, 1.0f); */
        /* SetVec3(gLightShader, "uLightColor", glm::value_ptr(light_color)); */

        SetMat4(gLightShader, "uView", glm::value_ptr(view));
        SetMat4(gLightShader, "uProj", glm::value_ptr(proj));

        glm::mat4 model(1.0f);
        model = glm::translate(model, gLightPosition);
        model = glm::scale(model, glm::vec3(0.2f));
        SetMat4(gLightShader, "uModel", glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
}

bool Update() {
    using namespace kdk;

    if (!PollWindowEvents()) {
        return false;
    }

    Update(&gFreeCamera, gFrameDelta);

    Render();

    return true;
}

int main() {
    using namespace kdk;

    if (!InitWindow(kWidth, kHeight)) {
        return -1;
    }
    DEFER { ShutdownWindow(); };

    kBasePath = SDL_GetCurrentDirectory();

    if (!InitRender()) {
        return -1;
    }

    glViewport(0, 0, kWidth, kHeight);

    // Use VSync.
    if (!SDL_GL_SetSwapInterval(1)) {
        SDL_Log("Unable to set VSYNC: %s\n", SDL_GetError());
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

        if (!SDL_GL_SwapWindow(gSDLWindow)) {
            SDL_Log("ERROR: Could not swap buffer: %s\n", SDL_GetError());
            return -1;
        }
    }

    return 0;
}

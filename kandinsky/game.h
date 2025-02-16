#pragma once

#include <kandinsky/defines.h>
#include <kandinsky/opengl.h>

namespace kdk {

struct PlatformState {
    i32 WindowWidth = 0;
    i32 WindowHeight = 0;

    u64 LastFrameTicks = 0;
    float FrameDelta = 0;

    std::string BasePath;

    LineBatcherRegistry LineBatchers = {};
    MeshRegistry Meshes = {};
    ShaderRegistry Shaders = {};
    TextureRegistry Textures = {};

    Camera FreeCamera = {
        .Position = glm::vec3(-4.0f, 1.0f, 1.0f),
    };

    glm::vec3 LightPosition = glm::vec3(1.2f, 1.0f, 2.0f);

    bool ShowDebugWindow = true;
};

bool GameInit(PlatformState* platform_state);
bool GameUpdate(PlatformState* platform_state);
void GameRender(PlatformState* platform_state);

}  // namespace kdk

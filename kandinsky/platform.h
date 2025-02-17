#pragma once

#include <kandinsky/defines.h>
#include <kandinsky/input.h>
#include <kandinsky/opengl.h>
#include <kandinsky/window.h>

#include <imgui.h>

#include <SDL3/SDL_loadso.h>

namespace kdk {

struct PlatformState {
    Window Window = {};
    InputState InputState = {};

    u64 LastFrameTicks = 0;
    float FrameDelta = 0;

    ImGuiContext *ImguiContext = nullptr;
    ImGuiMemAllocFunc ImguiAllocFunc = nullptr;
    ImGuiMemFreeFunc ImguiFreeFunc = nullptr;

    std::string BasePath;

    LineBatcherRegistry LineBatchers = {};
    LineBatcher *DebugLineBatcher = nullptr;

    MeshRegistry Meshes = {};
    ShaderRegistry Shaders = {};
    TextureRegistry Textures = {};

    Camera FreeCamera = {
        .Position = glm::vec3(-4.0f, 1.0f, 1.0f),
    };

    glm::vec3 LightPosition = glm::vec3(1.2f, 1.0f, 2.0f);

    bool ShowDebugWindow = true;
};

#ifdef __cplusplus
extern "C" {
#endif

__declspec(dllexport) bool OnSharedObjectLoaded(PlatformState *platform_state);
__declspec(dllexport) bool OnSharedObjectUnloaded(PlatformState *platform_state);
__declspec(dllexport) bool GameInit(PlatformState *platform_state);
__declspec(dllexport) bool GameUpdate(PlatformState *platform_state);
__declspec(dllexport) bool GameRender(PlatformState *platform_state);

#ifdef __cplusplus
}
#endif

struct LoadedGameLibrary {
    SDL_SharedObject *SO = nullptr;
    bool (*OnSharedObjectLoaded)(PlatformState *ps) = nullptr;
    bool (*OnSharedObjectUnloaded)(PlatformState *ps) = nullptr;
    bool (*GameInit)(PlatformState *ps) = nullptr;
    bool (*GameUpdate)(PlatformState *ps) = nullptr;
    bool (*GameRender)(PlatformState *ps) = nullptr;
};
bool IsValid(const LoadedGameLibrary &game_lib);

// Load the game library from a DLL and get the function pointers.
LoadedGameLibrary LoadGameLibrary(PlatformState *ps, const char *so_path);
void UnloadGameLibrary(PlatformState *ps, LoadedGameLibrary *lgl);

}  // namespace kdk

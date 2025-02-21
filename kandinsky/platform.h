#pragma once

#include <kandinsky/defines.h>
#include <kandinsky/input.h>
#include <kandinsky/memory.h>
#include <kandinsky/opengl.h>
#include <kandinsky/window.h>

#include <imgui.h>

#include <SDL3/SDL_loadso.h>

namespace kdk {

// LoadedGameLibrary -------------------------------------------------------------------------------

struct LoadedGameLibrary {
    SDL_SharedObject* SO = nullptr;
    SDL_Time SOModifiedTime = {};

    bool (*OnSharedObjectLoaded)(PlatformState* ps) = nullptr;
    bool (*OnSharedObjectUnloaded)(PlatformState* ps) = nullptr;
    bool (*GameInit)(PlatformState* ps) = nullptr;
    bool (*GameUpdate)(PlatformState* ps) = nullptr;
    bool (*GameRender)(PlatformState* ps) = nullptr;
};
bool IsValid(const LoadedGameLibrary& game_lib);

// Load the game library from a DLL and get the function pointers.
bool CheckForNewGameLibrary(PlatformState* ps, const char* so_path);

// Will load it into the PlatformState.
bool LoadGameLibrary(PlatformState* ps, const char* so_path);
bool UnloadGameLibrary(PlatformState* ps);

// PlatformState -----------------------------------------------------------------------------------

struct PlatformState {
    std::string BasePath;

    Window Window = {};
    InputState InputState = {};

    u64 LastFrameTicks = 0;
    double FrameDelta = 0;

    struct Memory {
		Arena PermanentArena = {};
        Arena FrameArena = {};
        // Note that all strings allocated into this arena are *permanent*.
        Arena StringArena = {};
    } Memory;

    struct GameLibrary {
        const char* Path = nullptr;
        LoadedGameLibrary LoadedLibrary = {};
        SDL_Time LastLoadTime = 0;
    } GameLibrary;

    struct Imgui {
        ImGuiContext* Context = nullptr;
        ImGuiMemAllocFunc AllocFunc = nullptr;
        ImGuiMemFreeFunc FreeFunc = nullptr;
    } Imgui;

    LineBatcherRegistry LineBatchers = {};
    LineBatcher* DebugLineBatcher = nullptr;

    MeshRegistry Meshes = {};

    struct Shaders {
        ShaderRegistry Registry = {};
        SDL_Time LastLoadTime = 0;
    } Shaders;

    TextureRegistry Textures = {};

    void* GameState = nullptr;
};

struct InitPlatformConfig {
    const char* WindowName = nullptr;
    int WindowWidth = 1440;
    int WindowHeight = 1080;

    const char* GameLibraryPath = nullptr;
};

bool InitPlatform(PlatformState* ps, const InitPlatformConfig& config);
void ShutdownPlatform(PlatformState* ps);

// This will re-evaluate the state of the platform, and reload resources appropiatelly.
bool ReevaluatePlatform(PlatformState* ps);

#ifdef __cplusplus
extern "C" {
#endif

__declspec(dllexport) bool OnSharedObjectLoaded(PlatformState* platform_state);
__declspec(dllexport) bool OnSharedObjectUnloaded(PlatformState* platform_state);
__declspec(dllexport) bool GameInit(PlatformState* platform_state);
__declspec(dllexport) bool GameUpdate(PlatformState* platform_state);
__declspec(dllexport) bool GameRender(PlatformState* platform_state);

#ifdef __cplusplus
}
#endif

}  // namespace kdk

#pragma once

#include <kandinsky/defines.h>
#include <kandinsky/graphics/opengl.h>
#include <kandinsky/input.h>
#include <kandinsky/memory.h>
#include <kandinsky/window.h>

#include <imgui.h>

#include <SDL3/SDL_loadso.h>

namespace kdk {

// PlatformState -----------------------------------------------------------------------------------

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

struct PlatformState {
    String BasePath;

    Window Window = {};
    InputState InputState = {};

    u64 LastFrameTicks = 0;
    double Seconds = 0;
    double FrameDelta = 0;

    struct Functions {
        void (*RenderImgui)() = nullptr;
    } Functions;

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

    MaterialRegistry Materials = {};
    MeshRegistry Meshes = {};
    ModelRegistry Models = {};

    struct Shaders {
        ShaderRegistry Registry = {};
        SDL_Time LastLoadTime = 0;

        struct {
            Shader* Grid = nullptr;
            u32 GridVAO = 0;
        } SystemShaders;
    } Shaders;

    TextureRegistry Textures = {};

    void* GameState = nullptr;
};

namespace platform {

PlatformState* GetPlatformContext();
// Use for newly loaded DLLs.
void SetPlatformContext(PlatformState* ps);

Arena* GetFrameArena();
Arena* GetPermanentArena();

Arena* GetStringArena();
String InternToStringArena(const char* string);

}  // namespace platform

#ifdef __cplusplus
extern "C" {
#endif

__declspec(dllexport) bool OnSharedObjectLoaded(PlatformState* ps);
__declspec(dllexport) bool OnSharedObjectUnloaded(PlatformState* ps);
__declspec(dllexport) bool GameInit(PlatformState* ps);
__declspec(dllexport) bool GameUpdate(PlatformState* ps);
__declspec(dllexport) bool GameRender(PlatformState* ps);

#ifdef __cplusplus
}
#endif

}  // namespace kdk

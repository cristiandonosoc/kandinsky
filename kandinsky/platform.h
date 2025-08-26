#pragma once

#include <kandinsky/asset_registry.h>
#include <kandinsky/core/defines.h>
#include <kandinsky/core/memory.h>
#include <kandinsky/graphics/model.h>
#include <kandinsky/graphics/opengl.h>
#include <kandinsky/graphics/render_state.h>
#include <kandinsky/input.h>
#include <kandinsky/scene.h>
#include <kandinsky/window.h>

#include <imgui.h>

#include <SDL3/SDL_loadso.h>

namespace kdk {

// PlatformState -----------------------------------------------------------------------------------

struct LoadedGameLibrary {
    SDL_SharedObject* SO = nullptr;
    SDL_Time SOModifiedTime = {};

    bool (*__KDKEntryPoint_OnSharedObjectLoaded)(PlatformState* ps) = nullptr;
    bool (*__KDKEntryPoint_OnSharedObjectUnloaded)(PlatformState* ps) = nullptr;
    bool (*__KDKEntryPoint_GameInit)(PlatformState* ps) = nullptr;
    bool (*__KDKEntryPoint_GameUpdate)(PlatformState* ps) = nullptr;
    bool (*__KDKEntryPoint_GameRender)(PlatformState* ps) = nullptr;
};
bool IsValid(const LoadedGameLibrary& game_lib);

struct PlatformState {
    String BasePath;

    Window Window = {};
    InputState InputState = {};

    bool ShouldExit = false;

    u64 LastFrameTicks = 0;
    double Seconds = 0;
    double FrameDelta = 0;

    Vec3 ClearColor = Vec3(0.2f);

    bool MainCameraMode = true;
    Camera MainCamera = {};
    Camera DebugCamera = {};
    Camera* CurrentCamera = nullptr;

    // Debug FBO (for debug camera mode).
    GLuint DebugFBO = NULL;
    GLuint DebugFBOTexture = NULL;
    GLuint DebugFBODepthStencil = NULL;

    struct Memory {
        Arena PermanentArena = {};
        Arena FrameArena = {};
        // Note that all strings allocated into this arena are *permanent*.
        Arena StringArena = {};
		Arena AssetLoadingArena = {};
    } Memory;

    struct GameLibrary {
        String Path = {};
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

    Scene Scene = {};
    EntityManager* EntityManager = nullptr;

    EntityID SelectedEntityID = {};
    EntityID HoverEntityID = {};
    EntityPicker EntityPicker = {};


    ModelRegistry Models = {};
    TextureRegistry Textures = {};
    MaterialRegistry Materials = {};
    ShaderRegistry Shaders = {};

    SDL_Time Shaders_LastLoadTime = 0;

    AssetRegistry Assets = {};
    BaseAssets BaseAssets = {};

    // The current options of the scene being rendered.
    RenderState RenderState = {};

    void* GameState = nullptr;
};

struct SerdeContext {
    PlatformState* PlatformState = nullptr;
    EntityManager* EntityManager = nullptr;
    AssetRegistry* AssetRegistry = nullptr;
};
void FillSerdeContext(PlatformState* ps, SerdeContext* sc);

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

__declspec(dllexport) bool __KDKEntryPoint_OnSharedObjectLoaded(PlatformState* ps);
__declspec(dllexport) bool __KDKEntryPoint_OnSharedObjectUnloaded(PlatformState* ps);
__declspec(dllexport) bool __KDKEntryPoint_GameInit(PlatformState* ps);
__declspec(dllexport) bool __KDKEntryPoint_GameUpdate(PlatformState* ps);
__declspec(dllexport) bool __KDKEntryPoint_GameRender(PlatformState* ps);

#ifdef __cplusplus
}
#endif

}  // namespace kdk

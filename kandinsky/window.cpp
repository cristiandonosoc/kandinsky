#include <SDL3/SDL_log.h>
#include <kandinsky/window.h>

#include <kandinsky/debug.h>
#include <kandinsky/glew.h>
#include <kandinsky/input.h>
#include <kandinsky/platform.h>

#include <SDL3/SDL_system.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>

#include <nfd.hpp>

#include <chrono>
#include <format>
#include "kandinsky/core/string.h"

namespace kdk {

namespace window_private {

void* NFD_GetNativeWindowFromSDLWindow(SDL_Window* window) {
#ifdef PLATFORM_WINDOWS
    return (void*)SDL_GetPointerProperty(SDL_GetWindowProperties(window),
                                         SDL_PROP_WINDOW_WIN32_HWND_POINTER,
                                         NULL);
#endif  // PLATFORM WINDOWS

    return nullptr;
}

bool CheckForNewGameLibrary(PlatformState* ps) {
    SDL_PathInfo info = {};
    if (!SDL_GetPathInfo(ps->GameLibrary.Path.Str(), &info)) {
        return false;
    }

    if (ps->GameLibrary.LoadedLibrary.SOModifiedTime >= info.modify_time) {
        return false;
    }

    return true;
}

std::pair<bool, String> CreateNewLibraryLoadPath(Arena* arena, PlatformState* ps) {
    auto scratch = GetScratchArena(arena);

    // Copy the DLL to a temporary location.
    String temp_path = Printf(scratch, "%s/temp/game_dlls", ps->BasePath.Str());
    if (!SDL_CreateDirectory(temp_path.Str())) {
        SDL_Log("ERROR: Creating temp path %s", temp_path.Str());
        return {false, {}};
    }

    // TODO(cdc): Move this Printf.
    std::string new_path = std::format("{}\\test_{:%y%m%d_%H%M%S}.dll",
                                       temp_path.Str(),
                                       std::chrono::system_clock::now());
    String result = InternStringToArena(arena, new_path.c_str(), new_path.size());
    ASSERT(!result.IsEmpty());
    return {true, result};
}

bool LoadGameLibrary(PlatformState* ps) {
    LoadedGameLibrary lgl = {};

    // Get the current time of the DLL.
    ASSERT(!ps->GameLibrary.TargetLoadPath.IsEmpty());
    const char* so_path = ps->GameLibrary.TargetLoadPath.Str();
    SDL_PathInfo info = {};
    if (!SDL_GetPathInfo(so_path, &info)) {
        SDL_Log("ERROR: Getting path info for %s: %s", so_path, SDL_GetError());
        return false;
    }
    lgl.SOModifiedTime = info.modify_time;

    lgl.SO = SDL_LoadObject(so_path);
    if (!lgl.SO) {
        SDL_Log("ERROR: Could not find SO at \"%s\"", so_path);
        return false;
    }

#define LOAD_FUNCTION(lgl, function_name)                                       \
    {                                                                           \
        SDL_FunctionPointer pointer = SDL_LoadFunction(lgl.SO, #function_name); \
        if (pointer == NULL) {                                                  \
            SDL_Log("ERROR: Didn't find function " #function_name);             \
            return false;                                                       \
        }                                                                       \
        lgl.function_name = (bool (*)(PlatformState*))pointer;                  \
    }

    LOAD_FUNCTION(lgl, __KDKEntryPoint_OnSharedObjectLoaded);
    LOAD_FUNCTION(lgl, __KDKEntryPoint_OnSharedObjectUnloaded);
    LOAD_FUNCTION(lgl, __KDKEntryPoint_GameInit);
    LOAD_FUNCTION(lgl, __KDKEntryPoint_GameUpdate);
    LOAD_FUNCTION(lgl, __KDKEntryPoint_GameRender);

#undef LOAD_FUNCTION

    if (!IsValid(lgl)) {
        SDL_Log("ERROR: LoadedGameLibrary is not valid!");
        SDL_UnloadObject(lgl.SO);
        return false;
    }

    SDL_Log("Loaded DLL at %s", so_path);
    if (!lgl.__KDKEntryPoint_OnSharedObjectLoaded(ps)) {
        SDL_Log("ERROR: Calling DLLInit on loaded DLL");
        SDL_UnloadObject(lgl.SO);
        return false;
    }

    ps->GameLibrary.LoadedLibrary = std::move(lgl);

    return true;
}

bool UnloadGameLibrary(PlatformState* ps) {
    if (!IsValid(ps->GameLibrary.LoadedLibrary)) {
        return false;
    }

    bool success = true;
    if (!ps->GameLibrary.LoadedLibrary.__KDKEntryPoint_OnSharedObjectUnloaded(ps)) {
        success = false;
    }

    SDL_UnloadObject(ps->GameLibrary.LoadedLibrary.SO);
    ps->GameLibrary.LoadedLibrary = {};

    return success;
}

bool InitialGameLibraryLoad(PlatformState* ps) {
    // We only wanna load so many libraries in a period of time.
    // This is just to avoid loading spurts.
    double now = ps->EditorTimeTracking.TotalSeconds;

    // Now that now that we should load a new library, we see if we have a target path for it.
    if (ps->GameLibrary.TargetLoadPath.IsEmpty()) {
        auto [ok, new_path] = window_private::CreateNewLibraryLoadPath(&ps->Memory.FrameArena, ps);
        if (!ok) {
            SDL_Log("ERROR: Unable create a new path for loading a new library\n");
            return false;
        }
        ps->GameLibrary.TargetLoadPath = new_path;
    }

    // Copy the library to the new location.
    if (!SDL_CopyFile(ps->GameLibrary.Path.Str(), ps->GameLibrary.TargetLoadPath.Str())) {
        return false;
    }

    if (!LoadGameLibrary(ps)) {
        SDL_Log("ERROR: Re-loading game library");
        return false;
    }

    // We finally succeeded, so we can reset the counters.
    ps->GameLibrary.LastLoadTime = now;
    ps->GameLibrary.LoadAttempStart = 0;
    ps->GameLibrary.TargetLoadPath = {};

    return true;
}

bool ReevaluateGameLibrary(PlatformState* ps) {
    // We only wanna load so many libraries in a period of time.
    // This is just to avoid loading spurts.
    double now = ps->EditorTimeTracking.TotalSeconds;

    double last_load_threshold =
        ps->GameLibrary.LastLoadTime + PlatformState::GameLibrary::kLoadThresholdSeconds;
    if (now < last_load_threshold) {
        return true;
    }

    // Check if we found a new version of the DLL.
    if (!CheckForNewGameLibrary(ps)) {
        return true;
    }

    // Sometimes build systems touch the SO before it is ready (or they do in succession).
    // We wait for a certain amount of frames since we detect it outdated to give a chance to
    // the build system to do its thing.
    {
        static u32 gSafetyFrames = 0;
        static constexpr u32 kSafetyFrameThreshold = 20;

        gSafetyFrames++;
        if (gSafetyFrames < kSafetyFrameThreshold) {
            return true;
        }
        gSafetyFrames = 0;
    }

    // Now that now that we should load a new library, we see if we have a target path for it.
    if (ps->GameLibrary.TargetLoadPath.IsEmpty()) {
        auto [ok, new_path] = window_private::CreateNewLibraryLoadPath(&ps->Memory.FrameArena, ps);
        if (!ok) {
            SDL_Log("ERROR: Unable create a new path for loading a new library\n");
            return false;
        }
        ps->GameLibrary.TargetLoadPath = new_path;
    }

    // Now we check if we just started loading, and is so we start the timer.
    if (ps->GameLibrary.LoadAttempStart == 0) {
        ps->GameLibrary.LoadAttempStart = now;
    }

    // We check the timer to see if we haven't exceeded it.
    double load_attempt_threshold =
        ps->GameLibrary.LoadAttempStart + PlatformState::GameLibrary::kMaxLoadTimeSeconds;
    if (now > load_attempt_threshold) {
        SDL_Log("ERROR: Exceeded loading threshold for loading a new library\n");
        return false;
    }

    // Attempt to copy the new library to the new attemp.
    //
    // NOTE: This can fail if the compiler still has the handle.
    //		 This is why we try again for several frames with the time threshold.
    if (!SDL_CopyFile(ps->GameLibrary.Path.Str(), ps->GameLibrary.TargetLoadPath.Str())) {
        return true;
    }

    // Now that we copied, we can un load the game library and load it again.

    if (!UnloadGameLibrary(ps)) {
        SDL_Log("ERROR: Unloading game library");
        return false;
    }

    if (!LoadGameLibrary(ps)) {
        SDL_Log("ERROR: Re-loading game library");
        return false;
    }

    // We finally succeeded, so we can reset the counters.
    ps->GameLibrary.LastLoadTime = now;
    ps->GameLibrary.LoadAttempStart = 0;
    ps->GameLibrary.TargetLoadPath = {};

    return true;
}

}  // namespace window_private

bool InitWindow(PlatformState* ps, const char* window_name, int width, int height) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        SDL_Log("ERROR: Initializing SDL: %s\n", SDL_GetError());
        return false;
    }

    ps->BasePath = paths::GetBaseDir(platform::GetStringArena());
    SDL_Log("Running from: %s", ps->BasePath.Str());

    // Setup window.
    SDL_Window* sdl_window = SDL_CreateWindow(window_name, width, height, SDL_WINDOW_OPENGL);
    if (!sdl_window) {
        SDL_Log("ERROR: Creating SDL Window: %s\n", SDL_GetError());
        return false;
    }

    void* native_handle = window_private::NFD_GetNativeWindowFromSDLWindow(sdl_window);
    if (!native_handle) {
        SDL_Log("ERROR: Getting native window handle from SDL window");
        return false;
    }

    SDL_SetWindowPosition(sdl_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    ps->InputState.KeyboardState = SDL_GetKeyboardState(nullptr);
    if (!ps->InputState.KeyboardState) {
        SDL_Log("ERROR: Getting keyboard state array");
        return false;
    }

    // Setup SDL Context.
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_GLContext gl_context = SDL_GL_CreateContext(sdl_window);
    if (gl_context == NULL) {
        SDL_Log("ERROR: Creating OpenGL Context: %s\n", SDL_GetError());
        return false;
    }
    SDL_GL_MakeCurrent(sdl_window, gl_context);

    // Initialize GLEW.
    if (!InitGlew(ps)) {
        return false;
    }

    // Use VSync.
    if (!SDL_GL_SetSwapInterval(1)) {
        SDL_Log("Unable to set VSYNC: %s\n", SDL_GetError());
        return false;
    }

    SDL_ShowWindow(sdl_window);

    ps->Window = Window{
        .Name = window_name,
        .SDLWindow = sdl_window,
        .NativeWindowHandle = native_handle,
        .Width = width,
        .Height = height,
        .GLContext = gl_context,
        .GLSLVersion = glsl_version,
    };

    return true;
}

void ShutdownWindow(PlatformState* ps) {
    if (!IsValid(ps->Window)) {
        return;
    }

    if (ps->Window.GLContext != NULL) {
        SDL_GL_DestroyContext(ps->Window.GLContext);
    }

    if (ps->Window.SDLWindow) {
        SDL_DestroyWindow(ps->Window.SDLWindow);
        ps->Window.SDLWindow = nullptr;
    }
}

bool PollWindowEvents(PlatformState* ps) {
    bool found_mouse_event = false;
    SDL_Event event;

    ps->InputState.KeyPressed.reset();
    ps->InputState.KeyReleased.reset();

    ps->InputState.MousePressed.reset();
    ps->InputState.MouseReleased.reset();

    ps->InputState.MouseScroll = {};

    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);

        switch (event.type) {
            case SDL_EVENT_QUIT: return false;
            case SDL_EVENT_KEY_DOWN: {
                ps->InputState.KeyPressed[event.key.scancode] = true;
                ps->InputState.KeyDown[event.key.scancode] = true;
                break;
            }
            case SDL_EVENT_KEY_UP: {
                ps->InputState.KeyReleased[event.key.scancode] = true;
                ps->InputState.KeyDown[event.key.scancode] = false;
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                ps->InputState.MousePressed[event.button.button] = true;
                ps->InputState.MouseDown[event.button.button] = true;
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                ps->InputState.MouseDown[event.button.button] = false;
                ps->InputState.MouseReleased[event.button.button] = true;
                break;
            }

            case SDL_EVENT_MOUSE_MOTION: {
                found_mouse_event = true;
                ps->InputState.MousePosition = {event.motion.x, event.motion.y};
                ps->InputState.MousePositionGL = {event.motion.x,
                                                  ps->Window.Height - event.motion.y};
                ps->InputState.MouseMove = {event.motion.xrel, event.motion.yrel};
                ps->InputState.MouseState = event.motion.state;
                break;
            }
            case SDL_EVENT_MOUSE_WHEEL: {
                ps->InputState.MouseScroll.x += event.wheel.x;
                ps->InputState.MouseScroll.y += event.wheel.y;
                break;
            }
        }
    }

    if (!found_mouse_event) {
        ps->InputState.MouseMove = {};
    }

    auto& io = ImGui::GetIO();
    ps->InputState.KeyboardOverride = io.WantCaptureKeyboard;
    ps->InputState.MouseOverride = io.WantCaptureMouse;

    return true;
}

// Platform Handling -------------------------------------------------------------------------------

namespace window_private {

bool InitMemory(PlatformState* ps) {
    ps->Memory.PermanentArena = AllocateArena(100 * MEGABYTE);
    ps->Memory.FrameArena = AllocateArena(25 * MEGABYTE);
    ps->Memory.StringArena = AllocateArena(25 * MEGABYTE);
    ps->Memory.AssetLoadingArena = AllocateArena(100 * MEGABYTE);

    return true;
}

void ShutdownMemory(PlatformState* ps) {
    FreeArena(ps->Memory.AssetLoadingArena.GetPtr());
    FreeArena(ps->Memory.StringArena.GetPtr());
    FreeArena(ps->Memory.FrameArena.GetPtr());
    FreeArena(ps->Memory.PermanentArena.GetPtr());
}

bool InitGraphics(PlatformState* ps) {
    InitOpenGL(ps);
    return true;
}

void ShutdownGraphics(PlatformState* ps) { ShutdownOpenGL(ps); }

bool InitTimeTracking(PlatformState* ps) {
    Init(&ps->EditorTimeTracking, platform::GetCPUTicks());
    ps->CurrentTimeTracking = &ps->EditorTimeTracking;

    return true;
}

void ShutdownTimeTracking(PlatformState* ps) {
    ps->EditorTimeTracking = {};
    ps->RuntimeTimeTracking = {};
    ps->CurrentTimeTracking = nullptr;
}

bool InitSystems(PlatformState* ps) { return InitSystems(ps, &ps->Systems); }
void ShutdownSystems(PlatformState* ps) { ShutdownSystems(&ps->Systems); }

bool InitThirdPartySystems(PlatformState* ps) {
    if (auto result = NFD::Init(); result != NFD_OKAY) {
        SDL_Log("ERROR: Initializing NFD (third_party/nfd");
        return false;
    }

    if (!InitImgui(ps)) {
        SDL_Log("ERROR: Initializing imgui");
        return false;
    }

    return true;
}

void ShutdownThirdPartySystems(PlatformState* ps) {
    ShutdownImgui(ps);
    NFD::Quit();
}

bool InitAssets(PlatformState* ps) {
    if (!Init(ps, &ps->Assets)) {
        SDL_Log("ERROR: Initializing asset registry");
        return false;
    }

    // Mark the SHADER_MARKER as loaded now to void initial spurious reloading.
    bool ok = SDL_GetCurrentTime(&ps->ShaderLoading.LastMarkerTimestamp);
    ASSERT(ok);

    return true;
}

void ShutdownAssets(PlatformState* ps) { Shutdown(ps, &ps->Assets); }

bool ReevaluateShaders(PlatformState* ps) {
    // We only wanna load so many libraries in a period of time.
    // This is just to avoid loading spurts.
    double now = ps->EditorTimeTracking.TotalSeconds;

    double last_load_threshold =
        ps->ShaderLoading.LastLoadTime + PlatformState::ShaderLoading::kLoadThresholdSeconds;
    if (now < last_load_threshold) {
        return true;
    }

    // Check for the guard.
    {
        auto scratch = GetScratchArena();

        String path = Printf(scratch, "%s/SHADER_MARKER", ps->BasePath.Str());
        SDL_PathInfo marker_file;
        if (!SDL_GetPathInfo(path.Str(), &marker_file)) {
            SDL_Log("Could not check marker at %s: %s", path.Str(), SDL_GetError());
            return true;
        }

        if (ps->ShaderLoading.LastMarkerTimestamp > marker_file.modify_time) {
            return true;
        }
    }

    SDL_Log("Re-evaluating shaders");
    if (!ReevaluateShaders(&ps->Assets)) {
        return false;
    }
    ps->GameLibrary.LastLoadTime = now;
    bool ok = SDL_GetCurrentTime(&ps->ShaderLoading.LastMarkerTimestamp);
    ASSERT(ok);

    return true;
}

bool LoadAndInitScene(PlatformState* ps) {
    InitScene(&ps->EditorScene, ESceneType::Editor);
    InitScene(&ps->GameplayScene, ESceneType::Game);

    ps->EntityManager = &ps->EditorScene.EntityManager;
    return true;
}

}  // namespace window_private

bool InitPlatform(PlatformState* ps, const InitPlatformConfig& config) {
    using namespace window_private;

    platform::SetPlatformContext(ps);

    if (!InitMemory(ps)) {
        SDL_Log("ERROR: Initializing memory");
        return false;
    }

    if (!InitWindow(ps, config.WindowName, config.WindowWidth, config.WindowHeight)) {
        SDL_Log("ERROR: Initializing window");
        return false;
    }

    if (!InitGraphics(ps)) {
        SDL_Log("ERROR: Initializing graphics");
        return false;
    }

    if (!InitTimeTracking(ps)) {
        SDL_Log("ERROR: Initializing time tracking");
        return false;
    }

    if (!Debug::Init(ps)) {
        SDL_Log("ERROR: Initializing debug");
        return false;
    }

    if (!InitSystems(ps)) {
        SDL_Log("ERROR: Initializing systems");
        return false;
    }

    if (!InitThirdPartySystems(ps)) {
        SDL_Log("ERROR: Initializing third party systems");
        __debugbreak();
        return false;
    }

    if (!InitAssets(ps)) {
        SDL_Log("ERROR: Initializing assets");
        __debugbreak();
        return false;
    }

    ps->GameLibrary.Path = config.GameLibraryPath;
    if (!InitialGameLibraryLoad(ps)) {
        SDL_Log("ERROR: Loading the first library");
        __debugbreak();
        return false;
    }

    if (!LoadAndInitScene(ps)) {
        SDL_Log("ERROR: Loading and initializing scene");
        __debugbreak();
        return false;
    }

    if (!ps->GameLibrary.LoadedLibrary.__KDKEntryPoint_GameInit(ps)) {
        __debugbreak();
        return false;
    }

    return true;
}

void ShutdownPlatform(PlatformState* ps) {
    using namespace window_private;

    UnloadGameLibrary(ps);
    ShutdownAssets(ps);
    ShutdownThirdPartySystems(ps);
    ShutdownSystems(ps);
    Debug::Shutdown(ps);
    ShutdownTimeTracking(ps);
    ShutdownGraphics(ps);
    ShutdownWindow(ps);
    ShutdownMemory(ps);

    platform::SetPlatformContext(nullptr);
}

bool ReevaluatePlatform(PlatformState* ps) {
    if (!window_private::ReevaluateGameLibrary(ps)) {
        ASSERT(false);
        return false;
    }

    if (!window_private::ReevaluateShaders(ps)) {
        ASSERT(false);
        return false;
    }

    return true;
}

// LoadedGameLibrary -------------------------------------------------------------------------------

bool IsValid(const LoadedGameLibrary& lgl) {
    // clang-format off
    return lgl.SO != nullptr &&
		   lgl.__KDKEntryPoint_OnSharedObjectLoaded != nullptr &&
		   lgl.__KDKEntryPoint_OnSharedObjectUnloaded != nullptr &&
		   lgl.__KDKEntryPoint_GameInit != nullptr &&
           lgl.__KDKEntryPoint_GameUpdate != nullptr &&
		   lgl.__KDKEntryPoint_GameRender != nullptr;
    // clang-format on
}

}  // namespace kdk

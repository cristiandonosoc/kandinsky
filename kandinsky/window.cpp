#include <kandinsky/window.h>

#include <kandinsky/debug.h>
#include <kandinsky/glew.h>
#include <kandinsky/input.h>
#include <kandinsky/platform.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>

#include <chrono>
#include <format>

namespace kdk {

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

    std::memset(&ps->InputState.MousePressed, 0, sizeof(ps->InputState.MousePressed));
    std::memset(&ps->InputState.KeyPressed, 0, sizeof(ps->InputState.KeyPressed));
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);

        if (event.type == SDL_EVENT_QUIT) {
            return false;
        }

        if (event.type == SDL_EVENT_KEY_UP) {
            // if (event.key.key == SDLK_ESCAPE) {
            //     return false;
            // }
            continue;
        }

        if (event.type == SDL_EVENT_MOUSE_MOTION) {
            found_mouse_event = true;
            ps->InputState.MousePosition = {event.motion.x, event.motion.y};
            ps->InputState.MousePositionGL = {event.motion.x, ps->Window.Height - event.motion.y};
            ps->InputState.MouseMove = {event.motion.xrel, event.motion.yrel};
            ps->InputState.MouseState = event.motion.state;
            continue;
        }

        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            ps->InputState.MousePressed[event.button.button] = true;
            continue;
        }

        if (event.type == SDL_EVENT_KEY_DOWN) {
            ps->InputState.KeyPressed[event.key.scancode] = true;
            continue;
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

namespace platform_private {

bool InitMemory(PlatformState* ps) {
    ps->Memory.PermanentArena = AllocateArena(100 * MEGABYTE);
    ps->Memory.FrameArena = AllocateArena(100 * MEGABYTE);
    ps->Memory.StringArena = AllocateArena(100 * MEGABYTE);

    return true;
}

void ShutdownMemory(PlatformState* ps) {
    FreeArena(&ps->Memory.StringArena);
    FreeArena(&ps->Memory.FrameArena);
    FreeArena(&ps->Memory.PermanentArena);
}

bool CheckForNewGameSO(PlatformState* ps) {
    // We only wanna load so many libraries in a period of time.
    // This is just to avoid loading spurts.
    constexpr SDL_Time kLoadThreshold = SDL_SECONDS_TO_NS(5);

    SDL_Time now = 0;
    bool ok = SDL_GetCurrentTime(&now);
    ASSERT(ok);

    if (ps->GameLibrary.LastLoadTime + kLoadThreshold > now) {
        return true;
    }

    if (!CheckForNewGameLibrary(ps, ps->GameLibrary.Path.Str())) {
        return true;
    }

    // Sometimes build systems touch the SO before it is ready (or they do in succession).
    // We wait for a certain amount of frames since we detect it outdated to give a chance to the
    // build system to do its thing.
    static u32 gOutdatedFrames = 0;
    constexpr u32 kOutdatedFrameThreshold = 20;

    gOutdatedFrames++;
    if (gOutdatedFrames < kOutdatedFrameThreshold) {
        return true;
    }
    gOutdatedFrames = 0;

    if (!UnloadGameLibrary(ps)) {
        SDL_Log("ERROR: Unloading game library");
        return false;
    }

    if (!LoadGameLibrary(ps, ps->GameLibrary.Path.Str())) {
        SDL_Log("ERROR: Re-loading game library");
        return false;
    }

    return true;
}

bool ReevaluateShaders(PlatformState* ps) {
    // We only wanna load so many libraries in a period of time.
    // This is just to avoid loading spurts.
    constexpr SDL_Time kLoadThreshold = SDL_SECONDS_TO_NS(5);

    SDL_Time now = 0;
    bool ok = SDL_GetCurrentTime(&now);
    ASSERT(ok);

    if (ps->Shaders.LastLoadTime + kLoadThreshold > now) {
        return true;
    }

    // Check for the guard.
    {
        String path = Printf(&ps->Memory.FrameArena, "%s/SHADER_MARKER", ps->BasePath.Str());
        SDL_PathInfo marker_file;
        if (!SDL_GetPathInfo(path.Str(), &marker_file)) {
            SDL_Log("Could not check marker at %s: %s", path.Str(), SDL_GetError());
            return true;
        }

        if (ps->Shaders.LastLoadTime > marker_file.modify_time) {
            return true;
        }
    }

    SDL_Log("Re-evaluating shaders");
    if (!ReevaluateShaders(&ps->Shaders.Registry)) {
        return false;
    }
    ok = SDL_GetCurrentTime(&ps->Shaders.LastLoadTime);
    ASSERT(ok);

    return true;
}

bool SetupFunctions(PlatformState* ps) {
    ps->Functions.RenderImgui = RenderImgui;
    return true;
}

}  // namespace platform_private

bool InitPlatform(PlatformState* ps, const InitPlatformConfig& config) {
    using namespace platform_private;

    platform::SetPlatformContext(ps);

    if (!InitMemory(ps)) {
        SDL_Log("ERROR: Initializing memory");
        return false;
    }

    if (!InitWindow(ps, config.WindowName, config.WindowWidth, config.WindowHeight)) {
        SDL_Log("ERROR: Initializing window");
        return false;
    }

    if (!Debug::Init(ps)) {
        SDL_Log("ERROR: Initializing debug");
        return false;
    }

    if (!InitImgui(ps)) {
        SDL_Log("ERROR: Initializing imgui");
        return false;
    }

    if (!SetupFunctions(ps)) {
        SDL_Log("ERROR: Setup functions");
        return false;
    }

    if (!LoadBaseAssets(ps)) {
        SDL_Log("ERROR: Loading initial assets");
		__debugbreak();
        return false;
    }

    ps->GameLibrary.Path = config.GameLibraryPath;
    if (!LoadGameLibrary(ps, ps->GameLibrary.Path.Str())) {
        SDL_Log("ERROR: Loading the first library");
		__debugbreak();
        return false;
    }

    if (!ps->GameLibrary.LoadedLibrary.GameInit(ps)) {
		__debugbreak();
        return false;
    }

    return true;
}

void ShutdownPlatform(PlatformState* ps) {
    using namespace platform_private;

    UnloadGameLibrary(ps);
    ShutdownImgui(ps);
    Debug::Shutdown(ps);
    ShutdownWindow(ps);
    ShutdownMemory(ps);

    platform::SetPlatformContext(nullptr);
}

bool ReevaluatePlatform(PlatformState* ps) {
    if (!platform_private::CheckForNewGameSO(ps)) {
        return false;
    }

    if (!platform_private::ReevaluateShaders(ps)) {
        return false;
    }

    return true;
}

// LoadedGameLibrary -------------------------------------------------------------------------------

bool IsValid(const LoadedGameLibrary& lgl) {
    // clang-format off
    return lgl.SO != nullptr &&
		   lgl.GameInit != nullptr &&
           lgl.GameUpdate != nullptr &&
		   lgl.GameRender != nullptr;
    // clang-format on
}

bool CheckForNewGameLibrary(PlatformState* ps, const char* so_path) {
    SDL_PathInfo info = {};
    if (!SDL_GetPathInfo(so_path, &info)) {
        return false;
    }

    if (ps->GameLibrary.LoadedLibrary.SOModifiedTime >= info.modify_time) {
        return false;
    }

    return true;
}

bool LoadGameLibrary(PlatformState* ps, const char* so_path) {
    LoadedGameLibrary lgl = {};

    // Get the current time of the DLL.
    SDL_PathInfo info = {};
    if (!SDL_GetPathInfo(so_path, &info)) {
        SDL_Log("ERROR: Getting path info for %s: %s", so_path, SDL_GetError());
        return false;
    }
    lgl.SOModifiedTime = info.modify_time;

    // Copy the DLL to a temporary location.
    String temp_path = Printf(&ps->Memory.FrameArena, "%s/temp/game_dlls", ps->BasePath.Str());
    if (!SDL_CreateDirectory(temp_path.Str())) {
        SDL_Log("ERROR: Creating temp path %s", temp_path.Str());
        return false;
    }

    // TODO(cdc): Move this Printf.
    std::string new_path = std::format("{}\\test_{:%y%m%d_%H%M%S}.dll",
                                       temp_path.Str(),
                                       std::chrono::system_clock::now());

    // We try for some times to copy the file, leaving some chance for the build system to free it.
    // At 60 FPS, this is waiting ~1s.
    bool copied = false;
    for (int i = 0; i < 60; i++) {
        if (!SDL_CopyFile(so_path, new_path.c_str())) {
            SDL_Delay(16);
            continue;
        }

        copied = true;
        break;
    }

    if (!copied) {
        SDL_Log("ERROR: Copying DLL from %s to %s: %s", so_path, new_path.c_str(), SDL_GetError());
        return false;
    }

    lgl.SO = SDL_LoadObject(new_path.c_str());
    if (!lgl.SO) {
        SDL_Log("ERROR: Could not find SO at \"%s\"", new_path.c_str());
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

    LOAD_FUNCTION(lgl, OnSharedObjectLoaded);
    LOAD_FUNCTION(lgl, OnSharedObjectUnloaded);
    LOAD_FUNCTION(lgl, GameInit);
    LOAD_FUNCTION(lgl, GameUpdate);
    LOAD_FUNCTION(lgl, GameRender);

#undef LOAD_FUNCTION

    if (!IsValid(lgl)) {
        SDL_Log("ERROR: LoadedGameLibrary is not valid!");
        SDL_UnloadObject(lgl.SO);
        return false;
    }

    SDL_Log("Loaded DLL at %s", new_path.c_str());
    if (!lgl.OnSharedObjectLoaded(ps)) {
        SDL_Log("ERROR: Calling DLLInit on loaded DLL");
        SDL_UnloadObject(lgl.SO);
        return false;
    }

    ps->GameLibrary.LoadedLibrary = std::move(lgl);
    bool ok = SDL_GetCurrentTime(&ps->GameLibrary.LastLoadTime);
    ASSERT(ok);

    return true;
}

bool UnloadGameLibrary(PlatformState* ps) {
    if (!IsValid(ps->GameLibrary.LoadedLibrary)) {
        return false;
    }

    bool success = true;
    if (!ps->GameLibrary.LoadedLibrary.OnSharedObjectUnloaded(ps)) {
        success = false;
    }

    SDL_UnloadObject(ps->GameLibrary.LoadedLibrary.SO);
    ps->GameLibrary.LoadedLibrary = {};

    return success;
}

}  // namespace kdk

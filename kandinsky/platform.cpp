#include <kandinsky/platform.h>

#include <kandinsky/debug.h>
#include <kandinsky/imgui.h>
#include <kandinsky/time.h>
#include <kandinsky/utils/defer.h>

#include <SDL3/SDL_log.h>

#include <chrono>
#include <format>

namespace kdk {

namespace platform_private {

bool CheckForNewGameSO(PlatformState* ps) {
    // We only wanna load so many libraries in a period of time.
    // This is just to avoid loading spurts.
    constexpr SDL_Time kLoadThreshold = SDL_SECONDS_TO_NS(5);

    SDL_Time now = 0;
    bool ok = SDL_GetCurrentTime(&now);
    assert(ok);

    if (ps->GameLibrary.LastLoadTime + kLoadThreshold > now) {
        return true;
    }

    if (!CheckForNewGameLibrary(ps, ps->GameLibrary.Path)) {
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

    if (!LoadGameLibrary(ps, ps->GameLibrary.Path)) {
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
    assert(ok);

    if (ps->Shaders.LastLoadTime + kLoadThreshold > now) {
        return true;
    }

    // Check for the guard.
    {
        std::string path = std::format("{}SHADER_MARKER", ps->BasePath.c_str());
        SDL_PathInfo marker_file;
        if (!SDL_GetPathInfo(path.c_str(), &marker_file)) {
            SDL_Log("Could not check marker at %s: %s", path.c_str(), SDL_GetError());
            return true;
        }

        if (ps->Shaders.LastLoadTime > marker_file.modify_time) {
            return true;
        }
    }

    SDL_Log("Re-evaluating shaders");
    if (!ReevaluateShaders(ps, &ps->Shaders.Registry)) {
        return false;
    }
    ok = SDL_GetCurrentTime(&ps->Shaders.LastLoadTime);
    assert(ok);

    return true;
}

}  // namespace platform_private

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
        SDL_Log("ERROR: Getting path info for %s", so_path);
        return false;
    }
    lgl.SOModifiedTime = info.modify_time;

    // Copy the DLL to a temporary location.
    std::string temp_path = std::format("{}temp\\game_dlls", ps->BasePath);
    if (!SDL_CreateDirectory(temp_path.c_str())) {
        SDL_Log("ERROR: Creating temp path %s", temp_path.c_str());
        return false;
    }

    std::string new_path =
        std::format("{}\\test_{:%y%m%d_%H%M%S}.dll", temp_path, std::chrono::system_clock::now());

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
    assert(ok);

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

bool InitPlatform(PlatformState* ps, const InitPlatformConfig& config) {
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

    ps->GameLibrary.Path = config.GameLibraryPath;
    if (!LoadGameLibrary(ps, ps->GameLibrary.Path)) {
        SDL_Log("ERROR: Loading the first library");
        return false;
    }

    if (!ps->GameLibrary.LoadedLibrary.GameInit(ps)) {
        return false;
    }

    return true;
}

void ShutdownPlatform(PlatformState* ps) {
    UnloadGameLibrary(ps);
    ShutdownImgui(ps);
    Debug::Shutdown(ps);
    ShutdownWindow(ps);
    ShutdownMemory(ps);
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
}  // namespace kdk

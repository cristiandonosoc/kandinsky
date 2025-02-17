#include <kandinsky/platform.h>

#include <kandinsky/debug.h>
#include <kandinsky/imgui.h>
#include <kandinsky/utils/defer.h>

#include <SDL3/SDL_log.h>

#include <chrono>
#include <format>

namespace kdk {

namespace platform_private {}  // namespace platform_private

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

    if (ps->LoadedGameLibrary.SOModifiedTime >= info.modify_time) {
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

    if (!lgl.OnSharedObjectLoaded(ps)) {
        SDL_Log("ERROR: Calling DLLInit on loaded DLL");
        SDL_UnloadObject(lgl.SO);
        return false;
    }

    SDL_Log("Loaded DLL at %s", new_path.c_str());

    ps->LoadedGameLibrary = std::move(lgl);
    if (!ps->LoadedGameLibrary.OnSharedObjectLoaded(ps)) {
        return false;
    }

    return true;
}

bool UnloadGameLibrary(PlatformState* ps) {
    if (!IsValid(ps->LoadedGameLibrary)) {
        return false;
    }

    bool success = true;
    if (!ps->LoadedGameLibrary.OnSharedObjectUnloaded(ps)) {
        success = false;
    }

    SDL_UnloadObject(ps->LoadedGameLibrary.SO);
    ps->LoadedGameLibrary = {};

    return success;
}

bool InitPlatform(PlatformState* ps, const InitPlatformConfig& config) {
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

    return true;
}

void ShutdownPlatform(PlatformState* ps) {
    ShutdownImgui(ps);
    Debug::Shutdown(ps);
    ShutdownWindow(ps);
}

}  // namespace kdk

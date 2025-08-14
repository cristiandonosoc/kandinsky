#pragma once

#include <kandinsky/core/defines.h>
#include <kandinsky/core/string.h>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

namespace kdk {

struct PlatformState;

struct Window {
    const char* Name = nullptr;
    SDL_Window* SDLWindow = nullptr;
	void* NativeWindowHandle = nullptr;
    i32 Width = 0;
    i32 Height = 0;

    SDL_GLContext GLContext = nullptr;
    const char* GLSLVersion = nullptr;
};
inline bool IsValid(const Window& window) { return window.SDLWindow != nullptr; }

bool InitWindow(PlatformState* ps, const char* window_name, int width, int height);
void ShutdownWindow(PlatformState* ps);

bool PollWindowEvents(PlatformState* ps);

// Platform Handling -------------------------------------------------------------------------------

struct InitPlatformConfig {
    const char* WindowName = nullptr;
    int WindowWidth = 1440;
    int WindowHeight = 1080;

    String GameLibraryPath = {};
};
bool InitPlatform(PlatformState* ps, const InitPlatformConfig& config);
void ShutdownPlatform(PlatformState* ps);

// This will re-evaluate the state of the platform, and reload resources appropiatelly.
bool ReevaluatePlatform(PlatformState* ps);

// LoadedGameLibrary -------------------------------------------------------------------------------

// Load the game library from a DLL and get the function pointers.
bool CheckForNewGameLibrary(PlatformState* ps, const char* so_path);

// Will load it into the PlatformState.
bool LoadGameLibrary(PlatformState* ps, const char* so_path);
bool UnloadGameLibrary(PlatformState* ps);

}  // namespace kdk

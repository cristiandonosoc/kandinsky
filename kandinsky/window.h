#pragma once

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <kandinsky/opengl.h>

namespace kdk {

struct PlatformState;

struct Window {
	const char* Name = nullptr;
    SDL_Window* SDLWindow = nullptr;
    i32 Width = 0;
    i32 Height = 0;

    SDL_GLContext GLContext = nullptr;
    const char* GLSLVersion = nullptr;
};
inline bool IsValid(const Window& window) { return window.SDLWindow != nullptr; }

bool InitWindow(PlatformState* ps, const char* window_name, int width, int height);
void ShutdownWindow(PlatformState* ps);

bool PollWindowEvents(PlatformState* ps);

}  // namespace kdk

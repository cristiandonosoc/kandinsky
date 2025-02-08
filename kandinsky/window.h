#pragma once

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <kandinsky/opengl.h>

namespace kdk {

extern SDL_Window* gSDLWindow;
extern SDL_GLContext gSDLGLContext;

bool InitWindow(int width, int height);
void ShutdownWindow();
bool PollWindowEvents();

}  // namespace kdk

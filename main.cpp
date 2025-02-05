#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

static SDL_Window *gSDLWindow = nullptr;
static bool gDone = false;

static constexpr int kWidth = 1024;
static constexpr int kHeight = 720;

bool Update() {
  SDL_Event e;
  if (SDL_PollEvent(&e)) {
    if (e.type == SDL_EVENT_QUIT) {
      return false;
    }

    if (e.type == SDL_EVENT_KEY_UP) {
      if (e.key.key == SDLK_ESCAPE) {
        return false;
      }
    }
  }

  SDL_Delay(1);
  return true;
}

int main() {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    SDL_Log("ERROR: Initializing");
    return -1;
  }

  gSDLWindow = SDL_CreateWindow("SDL3 window", kWidth, kHeight, 0);
  if (!gSDLWindow) {
    SDL_Log("ERROR: Window");
    return -1;
  }

  while (!gDone) {
    if (!Update()) {
      break;
    }
  }

  SDL_DestroyWindow(gSDLWindow);

  return 0;
}

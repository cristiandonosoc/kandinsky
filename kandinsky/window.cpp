#include <kandinsky/utils/defer.h>
#include <kandinsky/window.h>

namespace kdk {

SDL_Window* gSDLWindow = nullptr;
SDL_GLContext gSDLGLContext = GL_NONE;
const bool* gKeyboardState = nullptr;

bool InitWindow(int width, int height) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        SDL_Log("ERROR: Initializing SDL: %s\n", SDL_GetError());
        return false;
    }

    // Setup window.
    gSDLWindow = SDL_CreateWindow("SDL3 window", width, height, SDL_WINDOW_OPENGL);
    if (!gSDLWindow) {
        SDL_Log("ERROR: Creating SDL Window: %s\n", SDL_GetError());
        return false;
    }

    gKeyboardState = SDL_GetKeyboardState(nullptr);
    if (!gKeyboardState) {
        SDL_Log("ERROR: Getting keyboard state array");
        return false;
    }

    // Setup SDL Context.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    gSDLGLContext = SDL_GL_CreateContext(gSDLWindow);
    if (gSDLGLContext == NULL) {
        SDL_Log("ERROR: Creating OpenGL Context: %s\n", SDL_GetError());
        return false;
    }

    // Initialize GLEW.
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        SDL_Log("GLEW Init error: %s\n", glewGetErrorString(glewError));
        return false;
    }

    return true;
}

void ShutdownWindow() {
    if (gSDLGLContext != NULL) {
        SDL_GL_DestroyContext(gSDLGLContext);
    }

    if (gSDLWindow) {
        SDL_DestroyWindow(gSDLWindow);
        gSDLWindow = nullptr;
    }
}

bool PollWindowEvents() {
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

    return true;
}

}  // namespace kdk

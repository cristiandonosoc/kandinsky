#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <kandinsky/utils/defer.h>

// clang-format off
// We need this header ordering sadly.
#include <GL/glew.h>
#include <SDL3/SDL_opengl.h>
#include <GL/GLU.h>
// clang-format on

static SDL_Window* gSDLWindow = nullptr;
static SDL_GLContext gSDLGLContext = nullptr;

static bool gDone = false;

static constexpr int kWidth = 1024;
static constexpr int kHeight = 720;

namespace GL {

void PrintProgramLog(GLuint program);
void PrintShaderLog(GLuint shader);

}  // namespace GL

bool PollEvents() {
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

void Render() {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

bool Update() {
    if (!PollEvents()) {
        return false;
    }

    Render();

    SDL_Delay(1);
    return true;
}

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        SDL_Log("ERROR: Initializing SDL: %s\n", SDL_GetError());
        return -1;
    }

    // Setup window.
    gSDLWindow =
        SDL_CreateWindow("SDL3 window", kWidth, kHeight, SDL_WINDOW_OPENGL);
    if (!gSDLWindow) {
        SDL_Log("ERROR: Creating SDL Window: %s\n", SDL_GetError());
        return -1;
    }
    DEFER { SDL_DestroyWindow(gSDLWindow); };

    // Setup SDL Context.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);

    gSDLGLContext = SDL_GL_CreateContext(gSDLWindow);
    if (gSDLGLContext == NULL) {
        SDL_Log("ERROR: Creating OpenGL Context: %s\n", SDL_GetError());
        return -1;
    }
    DEFER { SDL_GL_DestroyContext(gSDLGLContext); };

    // Initialize GLEW.
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        SDL_Log("GLEW Init error: %s\n", glewGetErrorString(glewError));
        return -1;
    }

    glViewport(0, 0, kWidth, kHeight);

    // Use VSync.
    if (!SDL_GL_SetSwapInterval(1)) {
        SDL_Log("Unable to set VSYNC: %s\n", SDL_GetError());
        return -1;
    }

    while (!gDone) {
        if (!Update()) {
            break;
        }

        if (!SDL_GL_SwapWindow(gSDLWindow)) {
            SDL_Log("ERROR: Could not swap buffer: %s\n", SDL_GetError());
            return -1;
        }
    }

    return 0;
}

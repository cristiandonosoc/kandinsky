#include <kandinsky/window.h>

#include <kandinsky/glew.h>
#include <kandinsky/input.h>
#include <kandinsky/platform.h>
#include <kandinsky/utils/defer.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>

namespace kdk {

bool InitWindow(PlatformState* ps, const char* window_name, int width, int height) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        SDL_Log("ERROR: Initializing SDL: %s\n", SDL_GetError());
        return false;
    }

    ps->BasePath = SDL_GetCurrentDirectory();
    SDL_Log("Running from: %s", ps->BasePath.c_str());

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
    if (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);

        if (event.type == SDL_EVENT_QUIT) {
            return false;
        }

        if (event.type == SDL_EVENT_KEY_UP) {
            if (event.key.key == SDLK_ESCAPE) {
                return false;
            }
        }

        if (event.type == SDL_EVENT_MOUSE_MOTION) {
            found_mouse_event = true;
            ps->InputState.MousePosition = {event.motion.x, event.motion.y};
            ps->InputState.MouseMove = {event.motion.xrel, event.motion.yrel};
            ps->InputState.MouseState = event.motion.state;
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

}  // namespace kdk

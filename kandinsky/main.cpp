#include <kandinsky/debug.h>
#include <kandinsky/imgui.h>
#include <kandinsky/platform.h>
#include <kandinsky/utils/defer.h>
#include <kandinsky/window.h>

#include <SDL3/SDL_mouse.h>

kdk::PlatformState gPlatformState = {};

const char* kSOPath = "bazel-bin/apps/learn_opengl/learn_opengl_shared.dll";

bool Update() {
    u64 current_frame_ticks = SDL_GetTicksNS();
    if (gPlatformState.LastFrameTicks != 0) {
        u64 delta_ticks = current_frame_ticks - gPlatformState.LastFrameTicks;
        // Transform to seconds.
        gPlatformState.FrameDelta = static_cast<float>(delta_ticks) / 1'000'000'000.0f;
    }
    gPlatformState.LastFrameTicks = current_frame_ticks;

    kdk::BeginImguiFrame();

    kdk::Debug::StartFrame(&gPlatformState);

    if (!PollWindowEvents(&gPlatformState)) {
        return false;
    }

    return gPlatformState.GameLibrary.LoadedLibrary.GameUpdate(&gPlatformState);
}

bool Render() {
    if (!gPlatformState.GameLibrary.LoadedLibrary.GameRender(&gPlatformState)) {
        return false;
    }

    kdk::RenderImgui();

    return true;
}

int main() {
    using namespace kdk;

    if (!InitPlatform(&gPlatformState,
                      {
                          .WindowName = "Kandinsky",
                          .GameLibraryPath = kSOPath,
                      })) {
        SDL_Log("ERROR: Initializing platform");
        return -1;
    }
    DEFER { ShutdownPlatform(&gPlatformState); };

    while (true) {
        if (!ReevaluatePlatform(&gPlatformState)) {
            SDL_Log("ERROR: Re-evaluating platform");
            break;
        }

        if (!Update()) {
            break;
        }

        if (!Render()) {
            SDL_Log("ERROR: Render");
            break;
        }

        if (!SDL_GL_SwapWindow(gPlatformState.Window.SDLWindow)) {
            SDL_Log("ERROR: Could not swap buffer: %s\n", SDL_GetError());
            return -1;
        }
    }

    return 0;
}

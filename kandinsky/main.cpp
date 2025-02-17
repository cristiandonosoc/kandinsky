#include <kandinsky/debug.h>
#include <kandinsky/imgui.h>
#include <kandinsky/platform.h>
#include <kandinsky/utils/defer.h>
#include <kandinsky/window.h>

#include <SDL3/SDL_mouse.h>
#include <string>

static constexpr int kWidth = 1440;
static constexpr int kHeight = 1080;

kdk::PlatformState gPlatformState = {};

const char* kSOPath = "bazel-bin/kandinsky/apps/learn_opengl_shared.dll";

bool InitGame() {
    if (!kdk::LoadGameLibrary(&gPlatformState, kSOPath)) {
		SDL_Log("ERROR: Loading the first library");
        return false;
    }

    if (!gPlatformState.LoadedGameLibrary.GameInit(&gPlatformState)) {
        return false;
    }

    return true;
}

void ShutdownGame() { kdk::UnloadGameLibrary(&gPlatformState); }

bool CheckForNewGameSO() {
    if (!kdk::CheckForNewGameLibrary(&gPlatformState, kSOPath)) {
        return true;
    }

    if (!kdk::UnloadGameLibrary(&gPlatformState)) {
        SDL_Log("ERROR: Unloading game library");
        return false;
    }

    if (!kdk::LoadGameLibrary(&gPlatformState, kSOPath)) {
        SDL_Log("ERROR: Re-loading game library");
        return false;
    }

    return true;
}

bool Update() {
    u64 current_frame_ticks = SDL_GetTicksNS();
    if (gPlatformState.LastFrameTicks != 0) {
        u64 delta_ticks = current_frame_ticks - gPlatformState.LastFrameTicks;
        // Transform to seconds.
        gPlatformState.FrameDelta = static_cast<float>(delta_ticks) / 1'000'000'000.0f;
    }
    gPlatformState.LastFrameTicks = current_frame_ticks;

    if (!CheckForNewGameSO()) {
        return false;
    }

    kdk::BeginImguiFrame();

    kdk::Debug::StartFrame(&gPlatformState);

    if (!PollWindowEvents(&gPlatformState)) {
        return false;
    }

    return gPlatformState.LoadedGameLibrary.GameUpdate(&gPlatformState);
}

bool Render() {
    if (!gPlatformState.LoadedGameLibrary.GameRender(&gPlatformState)) {
        return false;
    }

    kdk::RenderImgui();

    return true;
}

int main() {
    using namespace kdk;

    if (!InitWindow(&gPlatformState, "kandinsky", kWidth, kHeight)) {
        SDL_Log("ERROR: Initializing window");
        return -1;
    }
    DEFER { ShutdownWindow(&gPlatformState); };

    SDL_Log("Running from: %s", gPlatformState.BasePath.c_str());

    if (!Debug::Init(&gPlatformState)) {
        SDL_Log("ERROR: Initializing debug");
        return -1;
    }
    DEFER { Debug::Shutdown(&gPlatformState); };

    if (!InitImgui(&gPlatformState)) {
        SDL_Log("ERROR: Initializing imgui");
        return -1;
    }
    DEFER { ShutdownImgui(&gPlatformState); };

    if (!InitGame()) {
        SDL_Log("Error: Initializing game");
        return -1;
    }
    DEFER { ShutdownGame(); };

    while (true) {
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

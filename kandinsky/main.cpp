#include <kandinsky/debug.h>
#include <kandinsky/game.h>
#include <kandinsky/imgui.h>
#include <kandinsky/utils/defer.h>
#include <kandinsky/window.h>

#include <SDL3/SDL_mouse.h>
#include <string>

static constexpr int kWidth = 1440;
static constexpr int kHeight = 1080;

kdk::PlatformState gPlatformState = {};
kdk::LoadedGameLibrary gLoadedGame = {};

bool InitGame() {
    const char* so_path = "bazel-bin/kandinsky/apps/learn_opengl_shared.dll";
    gLoadedGame = kdk::LoadGameLibrary(&gPlatformState, so_path);
    if (!IsValid(gLoadedGame)) {
        return false;
    }

    return gLoadedGame.GameInit(&gPlatformState);
}

void ShutdownGame() { kdk::UnloadGameLibrary(&gPlatformState, &gLoadedGame); }

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

    return gLoadedGame.GameUpdate(&gPlatformState);
}

void Render() {
    gLoadedGame.GameRender(&gPlatformState);

    kdk::RenderImgui();
}

int main() {
    using namespace kdk;

    if (!InitWindow(&gPlatformState, "kandinsky", kWidth, kHeight)) {
        SDL_Log("ERROR: Initializing window");
        return -1;
    }
    DEFER { ShutdownWindow(&gPlatformState); };

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

        Render();

        if (!SDL_GL_SwapWindow(gPlatformState.Window.SDLWindow)) {
            SDL_Log("ERROR: Could not swap buffer: %s\n", SDL_GetError());
            return -1;
        }
    }

    return 0;
}

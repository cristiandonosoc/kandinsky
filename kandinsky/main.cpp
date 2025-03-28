#include <kandinsky/debug.h>
#include <kandinsky/imgui.h>
#include <kandinsky/platform.h>
#include <kandinsky/utils/arg_parser.h>
#include <kandinsky/window.h>

#include <SDL3/SDL_mouse.h>
#include "kandinsky/memory.h"

kdk::PlatformState gPlatformState = {};

bool Update() {
    // TODO(cdc): Move this to platform.
    u64 current_frame_ticks = SDL_GetTicksNS();
    gPlatformState.Seconds = current_frame_ticks / 1'000'000'000.0f;
    if (gPlatformState.LastFrameTicks != 0) [[unlikely]] {
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

    return true;
}

bool EndFrame() {
    kdk::EndImguiFrame();
    // TODO(cdc): Move this to platform.
    kdk::ArenaReset(&gPlatformState.Memory.FrameArena);

    return true;
}

int main(int argc, const char* argv[]) {
    using namespace kdk;

    Arena init_arena = AllocateArena(64 * KILOBYTE);
    DEFER { FreeArena(&init_arena); };

    ArgParser ap = {};
    AddStringArgument(&ap, "shared_lib", NULL, true);

    if (!ParseArguments(&ap, argc, argv)) {
        printf("ERROR: parsing arguments");
        return -1;
    }

    String so_path;
    {
        const char* res = nullptr;
        bool ok = FindStringValue(ap, "shared_lib", &res);
        ASSERT(ok);

        so_path = String(res);
        if (!paths::IsAbsolute(so_path)) {
            so_path = paths::PathJoin(&init_arena, paths::GetBaseDir(&init_arena), so_path);
        }
    }

    if (!InitPlatform(&gPlatformState,
                      {
                          .WindowName = "Kandinsky",
                          .GameLibraryPath = so_path,
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

        if (!EndFrame()) {
            SDL_Log("ERROR: EndFrame");
            break;
        }

        if (!SDL_GL_SwapWindow(gPlatformState.Window.SDLWindow)) {
            SDL_Log("ERROR: Could not swap buffer: %s\n", SDL_GetError());
            return -1;
        }
    }

    return 0;
}

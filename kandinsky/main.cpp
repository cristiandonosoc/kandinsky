#include <kandinsky/core/memory.h>
#include <kandinsky/debug.h>
#include <kandinsky/imgui.h>
#include <kandinsky/platform.h>
#include <kandinsky/utils/arg_parser.h>
#include <kandinsky/window.h>

#include <SDL3/SDL_mouse.h>

namespace kdk {
namespace main_private {

PlatformState gPlatformState = {};

}  // namespace main_private
}  // namespace kdk

bool Update() {
    using namespace kdk;
    using namespace kdk::main_private;

    // TODO(cdc): Move this to platform.
    u64 current_frame_ticks = platform::GetCPUTicks();

    // We always update the editor frame tracking.
    Update(&gPlatformState.EditorTimeTracking, current_frame_ticks, gPlatformState.LastFrameTicks);

    switch (gPlatformState.EditorState.RunningMode) {
        case ERunningMode::Invalid: ASSERT(false); break;
        case ERunningMode::Editor: {
            // While in editor we clear the runtime timing.
            ResetStruct(&gPlatformState.RuntimeTimeTracking);
            break;
        }
        case ERunningMode::GameRunning: {
            Update(&gPlatformState.RuntimeTimeTracking,
                   current_frame_ticks,
                   gPlatformState.LastFrameTicks);
            break;
        }
        case ERunningMode::GamePaused: break;
        case ERunningMode::GameEndRequested: {
            EndPlay(&gPlatformState);
            break;
        }
        case ERunningMode::COUNT: ASSERT(false); break;
    }

    gPlatformState.LastFrameTicks = current_frame_ticks;

    BeginImguiFrame();

    Debug::StartFrame(&gPlatformState);

    if (!PollWindowEvents(&gPlatformState)) {
        return false;
    }

    if (gPlatformState.ShouldExit) {
        return false;
    }

    return gPlatformState.GameLibrary.LoadedLibrary.__KDKEntryPoint_GameUpdate(&gPlatformState);
}

bool Render() {
    using namespace kdk;
    using namespace kdk::main_private;

    if (!gPlatformState.GameLibrary.LoadedLibrary.__KDKEntryPoint_GameRender(&gPlatformState)) {
        return false;
    }

    RenderImgui();

    return true;
}

bool EndFrame() {
    using namespace kdk;
    using namespace kdk::main_private;

    EndImguiFrame();
    // TODO(cdc): Move this to platform.
    ArenaReset(&gPlatformState.Memory.FrameArena);

    return true;
}

int main(int argc, const char* argv[]) {
    using namespace kdk;
    using namespace kdk::main_private;

    Arena init_arena = AllocateArena("InitArena"sv, 64 * KILOBYTE);
    DEFER { FreeArena(&init_arena); };

    ArgParser ap = {};
    AddStringArgument(&ap, "shared_lib"sv, NULL, true);
    AddStringArgument(&ap, "load_scene"sv, NULL, false);

    if (!ParseArguments(&ap, argc, argv)) {
        printf("ERROR: parsing arguments");
        return -1;
    }

    String so_path;
    {
        bool ok = FindStringValue(ap, "shared_lib"sv, &so_path);
        ASSERT(ok);

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

    {
        String scene_path;
        if (FindStringValue(ap, "load_scene"sv, &scene_path)) {
            LoadScene(&gPlatformState, scene_path);
        }
    }

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

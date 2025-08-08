#include <kandinsky/platform.h>

#include <kandinsky/glew.h>

#include <ImGuizmo.h>

// This is the app harness that holds the entry point for the application.
// The engine will load this functions which will call into YOUR functions.
// This is so that we can do some initialization/per frame stuff before your code.

namespace kdk {
// Forward declarations of the app functions.
bool OnSharedObjectLoaded(PlatformState* ps);
bool OnSharedObjectUnloaded(PlatformState* ps);
bool GameInit(PlatformState* ps);
bool GameUpdate(PlatformState* ps);
bool GameRender(PlatformState* ps);

}  // namespace kdk

#ifdef __cplusplus
extern "C" {
#endif

namespace kdk {

bool __KDKEntryPoint_OnSharedObjectLoaded(PlatformState* ps) {
    platform::SetPlatformContext(ps);
    // GameState* gs = (GameState*)ps->GameState;

    SDL_GL_MakeCurrent(ps->Window.SDLWindow, ps->Window.GLContext);

    // Initialize GLEW.
    if (!InitGlew(ps)) {
        return false;
    }

    ImGui::SetCurrentContext(ps->Imgui.Context);
    ImGui::SetAllocatorFunctions(ps->Imgui.AllocFunc, ps->Imgui.FreeFunc);

    ImGuizmo::SetImGuiContext(ps->Imgui.Context);

    if (!OnSharedObjectLoaded(ps)) {
        return false;
    }

    SDL_Log("KDK App Harness: Game DLL Loaded");
    return true;
}

bool __KDKEntryPoint_OnSharedObjectUnloaded(PlatformState* ps) {
    if (!OnSharedObjectUnloaded(ps)) {
        return false;
    }

    SDL_Log("KDK App Harness: Game DLL Unloaded");
    return true;
}

bool __KDKEntryPoint_GameInit(PlatformState* ps) { return GameInit(ps); }

bool __KDKEntryPoint_GameUpdate(PlatformState* ps) {
    ImGuizmo::BeginFrame();
    ImGuizmo::Enable(true);
    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    return GameUpdate(ps);
}

bool __KDKEntryPoint_GameRender(PlatformState* ps) { return GameRender(ps); }

}  // namespace kdk

#ifdef __cplusplus
}
#endif

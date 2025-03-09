#include <tower_defense/tower_defense.h>

#include <kandinsky/glew.h>
#include <kandinsky/graphics/render_state.h>
#include <kandinsky/platform.h>

namespace kdk {

namespace tower_defense_private {

TowerDefense* gTowerDefense = nullptr;

}  // namespace tower_defense_private

TowerDefense* TowerDefense::GetTowerDefense() { return tower_defense_private::gTowerDefense; }

bool TowerDefense::OnSharedObjectLoaded(PlatformState* ps) {
    platform::SetPlatformContext(ps);
    SDL_GL_MakeCurrent(ps->Window.SDLWindow, ps->Window.GLContext);

    // Initialize GLEW.
    if (!InitGlew(ps)) {
        return false;
    }

    ImGui::SetCurrentContext(ps->Imgui.Context);
    ImGui::SetAllocatorFunctions(ps->Imgui.AllocFunc, ps->Imgui.FreeFunc);

    tower_defense_private::gTowerDefense = (TowerDefense*)ps->GameState;

    SDL_Log("Game DLL Loaded");

    return true;
}

bool TowerDefense::OnSharedObjectUnloaded(PlatformState*) { return true; }

bool TowerDefense::GameInit(PlatformState* ps) {
    TowerDefense* td = ArenaPush<TowerDefense>(platform::GetPermanentArena());
    *td = {};
    ps->GameState = td;
    tower_defense_private::gTowerDefense = td;

    return true;
}

bool TowerDefense::GameUpdate(PlatformState* ps) {
    auto* td = GetTowerDefense();

    Update(ps, &td->Camera, ps->FrameDelta);

    return true;
}

bool TowerDefense::GameRender(PlatformState* ps) {
    auto* td = GetTowerDefense();

    RenderState rs = {};
    rs.CameraPosition = td->Camera.Position;
    rs.M_View = td->Camera.View;
    rs.M_Proj = td->Camera.Proj;
    rs.M_ViewProj = td->Camera.ViewProj;

    glViewport(0, 0, ps->Window.Width, ps->Window.Height);

    glEnable(GL_DEPTH_TEST);

    glClearColor(0.3f, 0.3f, 0.6f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    DrawGrid(rs);

    return true;
}

}  // namespace kdk

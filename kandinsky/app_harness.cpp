#include <kandinsky/glew.h>
#include <kandinsky/graphics/render_state.h>
#include <kandinsky/imgui.h>
#include <kandinsky/platform.h>

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

bool __KDKEntryPoint_GameInit(PlatformState* ps) {
    Init(&ps->Memory.PermanentArena, &ps->EntityManager);
    Init(&ps->EntityPicker);

    // Init cameras.
    ps->MainCamera.WindowSize = {ps->Window.Width, ps->Window.Height};
    ps->MainCamera.CameraType = ECameraType::Free;
    ps->MainCamera.Position = {};
    ps->MainCamera.FreeCamera = {};
    ps->MainCamera.PerspectiveData = {};

    ps->DebugCamera.WindowSize = {ps->Window.Width, ps->Window.Height};
    ps->DebugCamera.CameraType = ECameraType::Free;
    ps->DebugCamera.FreeCamera = {};
    ps->DebugCamera.PerspectiveData = {
        .Far = 200.0f,
    };

    ps->CurrentCamera = &ps->MainCamera;

    // Init the FBO for the debug camera mode.
    glGenFramebuffers(1, &ps->DebugFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ps->DebugFBO);

    glGenTextures(1, &ps->DebugFBOTexture);
    glBindTexture(GL_TEXTURE_2D, ps->DebugFBOTexture);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGB,
                 ps->Window.Width,
                 ps->Window.Height,
                 0,
                 GL_RGB,
                 GL_UNSIGNED_BYTE,
                 NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D,
                           ps->DebugFBOTexture,
                           0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenRenderbuffers(1, &ps->DebugFBODepthStencil);
    glBindRenderbuffer(GL_RENDERBUFFER, ps->DebugFBODepthStencil);
    glRenderbufferStorage(GL_RENDERBUFFER,
                          GL_DEPTH24_STENCIL8,
                          ps->Window.Width,
                          ps->Window.Height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                              GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER,
                              ps->DebugFBODepthStencil);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return GameInit(ps);
}

bool __KDKEntryPoint_GameUpdate(PlatformState* ps) {
    ImGuizmo::BeginFrame();
    ImGuizmo::Enable(true);
    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    static bool show_entity_list_window = false;
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Entities")) {
            if (ImGui::MenuItem("List")) {
                show_entity_list_window = !show_entity_list_window;
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    if (show_entity_list_window) {
        if (ImGui::Begin("Entity List", &show_entity_list_window)) {
            // Build the entity list in ImGui.
            kdk::BuildEntityListImGui(ps, &ps->EntityManager);
            ImGui::End();
        }
    }

    return GameUpdate(ps);
}

// GAME RENDER -------------------------------------------------------------------------------------

namespace app_harness_private {

bool RenderScene(PlatformState* ps, const RenderStateOptions& options) {
    // Calculate the render state.
    ps->RenderState = {
        .Options = options,
    };
    SetPlatformState(&ps->RenderState, *ps);
    // TODO(cdc): We should be passing the time.
    ps->RenderState.Seconds = 0;
    // rs.Seconds = 0.5f * static_cast<float>(SDL_GetTicks()) / 1000.0f;
    SetCamera(&ps->RenderState, *ps->CurrentCamera);

    // Set the lights.
    FixedArray<Light, 16> kLights = {};

    VisitComponents<DirectionalLightComponent>(
        &ps->EntityManager,
        [&kLights](EntityID, Entity*, DirectionalLightComponent* dl) {
            kLights.Push(Light{.LightType = dl->StaticLightType(), .DirectionalLight = dl});
            return true;
        });

    VisitComponents<PointLightComponent>(
        &ps->EntityManager,
        [&kLights](EntityID, Entity*, PointLightComponent* pl) {
            kLights.Push(Light{.LightType = pl->StaticLightType(), .PointLight = pl});
            return true;
        });

    VisitComponents<SpotlightComponent>(
        &ps->EntityManager,
        [&kLights](EntityID, Entity*, SpotlightComponent* sl) {
            kLights.Push(Light{.LightType = sl->StaticLightType(), .Spotlight = sl});
            return true;
        });

    std::span<Light> light_span(kLights.Data, kLights.Size);
    SetLights(&ps->RenderState, light_span);

    // Call the app.
    if (!GameRender(ps)) {
        return false;
    }

    DrawGrid(ps->RenderState);

    return true;
}

}  // namespace app_harness_private

bool __KDKEntryPoint_GameRender(PlatformState* ps) {
    // Clear the options.
    RenderStateOptions render_state_options = {};

    // Get the current camera and make sure to restore it.
    Camera* original_camera = ps->CurrentCamera;
    DEFER { ps->CurrentCamera = original_camera; };

    // Update the matrices of all the entities in the game.
    UpdateModelMatrices(&ps->EntityManager);

    glViewport(0, 0, ps->Window.Width, ps->Window.Height);

    if (ps->MainCameraMode) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Render the main camera.
        glEnable(GL_DEPTH_TEST);
        glClearColor(ps->ClearColor.r, ps->ClearColor.g, ps->ClearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);

        StartFrame(&ps->EntityPicker);
        if (!app_harness_private::RenderScene(ps, render_state_options)) {
            return false;
        }
        ps->HoverEntityID = EndFrame(&ps->EntityPicker);
    } else {
        // DEBUG CAMERA MODE.
        render_state_options.IsUsingDebugCamera = true;

        glBindFramebuffer(GL_FRAMEBUFFER, ps->DebugFBO);

        // Render the main camera.
        glEnable(GL_DEPTH_TEST);
        glClearColor(ps->ClearColor.r, ps->ClearColor.g, ps->ClearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);

        ps->CurrentCamera = &ps->MainCamera;
        if (!app_harness_private::RenderScene(ps, render_state_options)) {
            return false;
        }

        // Now we render the scene from the debug camera POV.
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Render the main camera.
        glEnable(GL_DEPTH_TEST);
        glClearColor(ps->ClearColor.r, ps->ClearColor.g, ps->ClearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);

        StartFrame(&ps->EntityPicker);

        ps->CurrentCamera = &ps->DebugCamera;
        if (!app_harness_private::RenderScene(ps, render_state_options)) {
            return false;
        }

        ps->HoverEntityID = EndFrame(&ps->EntityPicker);
    }

    return true;
}

}  // namespace kdk

#ifdef __cplusplus
}
#endif

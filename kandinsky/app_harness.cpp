#include <kandinsky/core/file.h>
#include <kandinsky/debug.h>
#include <kandinsky/entity.h>
#include <kandinsky/glew.h>
#include <kandinsky/graphics/render_state.h>
#include <kandinsky/imgui.h>
#include <kandinsky/platform.h>
#include <kandinsky/scene.h>

#include <glm/exponential.hpp>

#include <nfd.hpp>

#include <imgui.h>

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

// GAME INIT ---------------------------------------------------------------------------------------

bool __KDKEntryPoint_GameInit(PlatformState* ps) {
    Init(ps->EntityManager);
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

// GAME UPDATE -------------------------------------------------------------------------------------

namespace app_harness_private {

bool SaveSceneHandler(PlatformState* ps) {
    if (ps->Scene.Path.IsEmpty()) {
        NFD::UniquePath path;
        auto result = NFD::SaveDialog(path);
        if (result != NFD_OKAY) {
            NFD::GetError();
            SDL_Log("Error getting save file: %s", NFD::GetError());
            return false;
        }

        ps->Scene.Path.Set(path.get(), true);
    }

    SerdeArchive sa = NewSerdeArchive(&ps->Memory.FrameArena,
                                      &ps->Memory.FrameArena,
                                      ESerdeBackend::YAML,
                                      ESerdeMode::Serialize);
    SerdeContext sc = {};
    FillSerdeContext(ps, &sc);
    SetSerdeContext(&sa, &sc);
    Serde(&sa, "Scene", &ps->Scene);

    String yaml_str = GetSerializedString(&ps->Memory.FrameArena, sa);
    return SaveFile(ps->Scene.Path.ToString(), yaml_str.ToSpan());
}

bool LoadSceneHandler(PlatformState* ps) {
    NFD::UniquePath nfd_path;
    std::array filters = {
        nfdfilteritem_t{"YAML", "yml,yaml"}
    };

    if (auto result = NFD::OpenDialog(nfd_path, filters.data(), (u32)filters.size());
        result != NFD_OKAY) {
        NFD::GetError();
        SDL_Log("Error getting load file: %s", NFD::GetError());
        return false;
    }

    String path(nfd_path.get());
    auto data = LoadFile(&ps->Memory.FrameArena, path, {.NullTerminate = false});
    if (data.empty()) {
        SDL_Log("Empty file read in %s", path.Str());
        return false;
    }

    SerdeArchive sa = NewSerdeArchive(&ps->Memory.PermanentArena,
                                      &ps->Memory.FrameArena,
                                      ESerdeBackend::YAML,
                                      ESerdeMode::Deserialize);
    Load(&sa, data);

    ResetStruct(&ps->Scene);

    SerdeContext sc = {};
    FillSerdeContext(ps, &sc);
    SetSerdeContext(&sa, &sc);
    Serde(&sa, "Scene", &ps->Scene);

    return true;
}

void BuildMainMenuBar(PlatformState* ps) {
    auto scratch = GetScratchArena();

    static bool show_entity_list_window = false;
    static bool show_entity_debugger_window = false;
    static bool show_camera_window = false;
    static bool show_input_window = false;
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save Scene")) {
                SaveSceneHandler(ps);
            }

            if (ImGui::MenuItem("Load Scene")) {
                LoadSceneHandler(ps);
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                ps->ShouldExit = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Entities")) {
            if (ImGui::MenuItem("List")) {
                show_entity_list_window = !show_entity_list_window;
            }

            if (ImGui::MenuItem("Debugger")) {
                show_entity_debugger_window = !show_entity_debugger_window;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Systems")) {
            if (ImGui::MenuItem("Camera")) {
                show_camera_window = !show_camera_window;
            }
            if (ImGui::MenuItem("Input")) {
                show_input_window = !show_input_window;
            }
            ImGui::EndMenu();
        }

        // FPS marker.
        {
            double fps = 1.0 / ps->FrameDelta;
            String marker = Printf(scratch.Arena,
                                   "Entities: %d, FPS: %.2f",
                                   ps->EntityManager->EntityCount,
                                   fps);
            float width = ImGui::CalcTextSize(marker.Str()).x + 20.0f;
            float available_width = ImGui::GetContentRegionAvail().x;

            ImGui::SameLine(ImGui::GetCursorPosX() + available_width - width);
            ImGui::Text("%s", marker.Str());
        }

        ImGui::EndMainMenuBar();
    }

    if (show_entity_list_window) {
        if (ImGui::Begin("Entity List", &show_entity_list_window)) {
            // Build the entity list in ImGui.
            BuildEntityListImGui(ps, ps->EntityManager);
            ImGui::End();
        }
    }

    if (show_entity_debugger_window) {
        if (ImGui::Begin("Entity Debugger", &show_entity_debugger_window)) {
            BuildEntityDebuggerImGui(ps, ps->EntityManager);
            ImGui::End();
        }
    }

    if (show_camera_window) {
        if (ImGui::Begin("Camera", &show_camera_window)) {
            ImGui::Text("Main Camera Mode: %s", ps->MainCameraMode ? "ON" : "OFF");
            if (ImGui::Button("Toggle Camera Mode")) {
                ps->MainCameraMode = !ps->MainCameraMode;
                SetupDebugCamera(ps->MainCamera, &ps->DebugCamera);
            }

            if (ImGui::TreeNodeEx("Camera Info", ImGuiTreeNodeFlags_Framed)) {
                BuildImGui(&ps->MainCamera, ps->MainCameraMode ? NULL : ps->DebugFBOTexture);
                ImGui::TreePop();
            }

            if (ImGui::TreeNodeEx("Debug Camera Info", ImGuiTreeNodeFlags_Framed)) {
                BuildImGui(&ps->DebugCamera);
                ImGui::TreePop();
            }

            ImGui::End();
        }
    }

    if (show_input_window) {
        if (ImGui::Begin("Input", &show_input_window)) {
            ImGui::InputFloat2("Mouse",
                               GetPtr(ps->InputState.MousePosition),
                               "%.3f",
                               ImGuiInputTextFlags_ReadOnly);
            ImGui::InputFloat2("Mouse (GL)",
                               GetPtr(ps->InputState.MousePositionGL),
                               "%.3f",
                               ImGuiInputTextFlags_ReadOnly);
            ImGui::Text("Mouse Button Pressed: %s", MOUSE_PRESSED(ps, LEFT) ? "Yes" : "No");
            ImGui::Text("Mouse Button Pressed: %s", MOUSE_PRESSED(ps, RIGHT) ? "Yes" : "No");
            ImGui::End();
        }
    }
}
}  // namespace app_harness_private

bool __KDKEntryPoint_GameUpdate(PlatformState* ps) {
    ImGuizmo::BeginFrame();
    ImGuizmo::Enable(true);
    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    app_harness_private::BuildMainMenuBar(ps);

    if (MOUSE_PRESSED(ps, LEFT)) {
        if (IsValid(*ps->EntityManager, ps->HoverEntityID)) {
            ps->SelectedEntityID = ps->HoverEntityID;
        }
    }

    // Update camera.
    {
        if (KEY_PRESSED(ps, SPACE)) {
            ps->MainCameraMode = !ps->MainCameraMode;
            SetupDebugCamera(ps->MainCamera, &ps->DebugCamera);
        }

        Update(ps, ps->CurrentCamera, ps->FrameDelta);
        Recalculate(&ps->MainCamera);
        Recalculate(&ps->DebugCamera);
        if (ps->MainCameraMode) {
            Update(ps, &ps->MainCamera, ps->FrameDelta);
        } else {
            Update(ps, &ps->DebugCamera, ps->FrameDelta);
        }
        ps->CurrentCamera = ps->MainCameraMode ? &ps->MainCamera : &ps->DebugCamera;
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
        ps->EntityManager,
        [&kLights](EntityID, Entity*, DirectionalLightComponent* dl) {
            kLights.Push(Light{.LightType = dl->StaticLightType(), .DirectionalLight = dl});
            return true;
        });

    VisitComponents<PointLightComponent>(
        ps->EntityManager,
        [&kLights](EntityID, Entity*, PointLightComponent* pl) {
            kLights.Push(Light{.LightType = pl->StaticLightType(), .PointLight = pl});
            return true;
        });

    VisitComponents<SpotlightComponent>(
        ps->EntityManager,
        [&kLights](EntityID, Entity*, SpotlightComponent* sl) {
            kLights.Push(Light{.LightType = sl->StaticLightType(), .Spotlight = sl});
            return true;
        });

    std::span<Light> light_span(kLights.Data, kLights.Size);
    SetLights(&ps->RenderState, light_span);

    // Render the lights.
    Use(*ps->BaseAssets.LightShader);
    for (const Light& light : light_span) {
        if (light.LightType == ELightType::Point) {
            // SetVec2(*ps->BaseAssets.LightShader, "uMouseCoords", ps->InputState.MousePositionGL);
            // SetUVec2(*ps->BaseAssets.LightShader, "uObjectID", it->Entity.EditorID.ToUVec2());
            SetVec3(*ps->BaseAssets.LightShader, "uColor", Vec3(1.0f));
            SetEntity(&ps->RenderState, light.PointLight->GetOwnerID());
            ChangeModelMatrix(&ps->RenderState, light.PointLight->GetOwner()->M_Model);
            Draw(&ps->Assets,
                 ps->BaseAssets.CubeModelHandle,
                 *ps->BaseAssets.LightShader,
                 ps->RenderState);
        }
    }

    // Render the static models.
    Use(*ps->BaseAssets.NormalShader);
    auto static_models = GetEntitiesWithComponent<StaticModelComponent>(ps->EntityManager);
    for (EntityID id : static_models) {
        Entity* entity = GetEntity(ps->EntityManager, id);
        ASSERT(entity);
        StaticModelComponent* smc =
            GetComponent<StaticModelComponent>(ps->EntityManager, id).second;
        ASSERT(smc);
        SetVec3(*ps->BaseAssets.LightShader, "uColor", Vec3(1.0f));
        SetEntity(&ps->RenderState, id);
        ChangeModelMatrix(&ps->RenderState, entity->M_Model);
        Draw(&ps->Assets, smc->ModelHandle, *ps->BaseAssets.NormalShader, ps->RenderState);
    }

    SetEntity(&ps->RenderState, {});

    // Call the app.
    if (!GameRender(ps)) {
        return false;
    }

    // Draw main the camera is we're in debug camera mode.
    if (ps->RenderState.Options.IsUsingDebugCamera) {
        Use(*ps->BaseAssets.LightShader);

        Mat4 mmodel(1.0f);
        mmodel = Translate(mmodel, ps->MainCamera.Position);
        mmodel = Scale(mmodel, Vec3(0.1f));
        ChangeModelMatrix(&ps->RenderState, mmodel);

        Color32 color = Color32::MandarianOrange;
        SetVec3(*ps->BaseAssets.LightShader, "uColor", ToVec3(color));
        Draw(&ps->Assets,
             ps->BaseAssets.CubeModelHandle,
             *ps->BaseAssets.LightShader,
             ps->RenderState);

        Debug::DrawFrustum(ps, ps->MainCamera.M_ViewProj, color, 3);
    }

    Debug::Render(ps, *ps->BaseAssets.LineBatcherShader, ps->CurrentCamera->M_ViewProj);
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
    UpdateModelMatrices(ps->EntityManager);

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

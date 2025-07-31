#include <learn_opengl/learn_opengl.h>

#include <kandinsky/debug.h>
#include <kandinsky/defines.h>
#include <kandinsky/glew.h>
#include <kandinsky/graphics/render_state.h>
#include <kandinsky/imgui.h>
#include <kandinsky/input.h>
#include <kandinsky/math.h>
#include <kandinsky/platform.h>
#include <kandinsky/string.h>
#include <kandinsky/window.h>

#include <SDL3/SDL_mouse.h>

#include <ImGuizmo.h>
#include <imgui.h>

#include <string>

#ifdef __cplusplus
extern "C" {
#endif

namespace kdk {

Vec3 kCubePositions[] = {Vec3(0.0f, 0.0f, 0.0f),
                         Vec3(2.0f, 5.0f, -15.0f),
                         Vec3(-1.5f, -2.2f, -2.5f),
                         Vec3(-3.8f, -2.0f, -12.3f),
                         Vec3(2.4f, -0.4f, -3.5f),
                         Vec3(-1.7f, 3.0f, -7.5f),
                         Vec3(1.3f, -2.0f, -2.5f),
                         Vec3(1.5f, 2.0f, -2.5f),
                         Vec3(1.5f, 0.2f, -1.5f),
                         Vec3(-1.3f, 1.0f, -1.5f)};
// clang-format on

bool OnSharedObjectLoaded(PlatformState* ps) {
    platform::SetPlatformContext(ps);
    // GameState* gs = (GameState*)ps->GameState;

    SDL_GL_MakeCurrent(ps->Window.SDLWindow, ps->Window.GLContext);

    // Initialize GLEW.
    if (!InitGlew(ps)) {
        return false;
    }

    ImGui::SetCurrentContext(ps->Imgui.Context);
    ImGui::SetAllocatorFunctions(ps->Imgui.AllocFunc, ps->Imgui.FreeFunc);

    SDL_Log("Game DLL Loaded");

    return true;
}

bool OnSharedObjectUnloaded(PlatformState*) {
    SDL_Log("Game DLL Unloaded");
    return true;
}

bool GameInit(PlatformState* ps) {
    auto scratch = GetScratchArena();

    GameState* gs = (GameState*)ArenaPush(&ps->Memory.PermanentArena, sizeof(GameState));
    *gs = {};

    Init(&ps->Memory.PermanentArena, &gs->EntityManager);

    gs->MainCamera.WindowSize = {ps->Window.Width, ps->Window.Height};
    gs->MainCamera.CameraType = ECameraType::Free;
    gs->MainCamera.Position = Vec3(-4.0f, 1.0f, 1.0f);
    gs->MainCamera.FreeCamera = {};
    gs->MainCamera.PerspectiveData = {};

    gs->DebugCamera.WindowSize = {ps->Window.Width, ps->Window.Height};
    gs->DebugCamera.CameraType = ECameraType::Free;
    gs->DebugCamera.FreeCamera = {};
    gs->DebugCamera.PerspectiveData = {
        .Far = 200.0f,
    };

    gs->CurrentCamera = &gs->MainCamera;

    {
        auto [id, entity] = CreateEntity(&gs->EntityManager);
        auto [_, dl] = AddComponent<DirectionalLightComponent>(&gs->EntityManager, id);
        gs->DirectionalLight = dl;

        dl->Direction = Vec3(-1.0f, -1.0f, -1.0f);
        dl->Color = {
            .Ambient = Vec3(0.05f),
            .Diffuse = Vec3(0.4f),
            .Specular = Vec3(0.05f),
        };
    }

    {
        for (u64 i = 0; i < std::size(gs->PointLights); i++) {
            auto [id, entity] = CreateEntity(&gs->EntityManager);
            auto [_, pl] = AddComponent<PointLightComponent>(&gs->EntityManager, id);
            gs->PointLights[i] = pl;

            pl->Color = {.Ambient = Vec3(0.05f), .Diffuse = Vec3(0.8f), .Specular = Vec3(1.0f)};
        }

        gs->PointLights[0]->GetOwner()->Transform.Position = Vec3(0.7f, 0.2f, 2.0f);
        gs->PointLights[1]->GetOwner()->Transform.Position = Vec3(2.3f, -3.3f, -4.0f);
        gs->PointLights[2]->GetOwner()->Transform.Position = Vec3(-4.0f, 2.0f, -12.0f);
        gs->PointLights[3]->GetOwner()->Transform.Position = Vec3(0.0f, 0.0f, -3.0f);
    }

    {
        auto [id, entity] = CreateEntity(&gs->EntityManager);
        auto [_, sl] = AddComponent<SpotlightComponent>(&gs->EntityManager, entity->ID);
        gs->Spotlight = sl;

        entity->Transform.Position = Vec3(-1.0f);
        sl->Target = Vec3(0);
        sl->Color = {.Ambient = Vec3(0.05f), .Diffuse = Vec3(0.8f), .Specular = Vec3(1.0f)};
    }

    ps->GameState = gs;

    String path =
        paths::PathJoin(scratch.Arena, ps->BasePath, String("assets/textures/container2.png"));
    Texture* diffuse_texture = CreateTexture(&ps->Textures,
                                             "DiffuseTexture",
                                             path.Str(),
                                             {
                                                 .Type = ETextureType::Diffuse,

                                             });
    if (!diffuse_texture) {
        SDL_Log("ERROR: Loading diffuse texture");
        return false;
    }

    path = paths::PathJoin(scratch.Arena,
                           ps->BasePath,
                           String("assets/textures/container2_specular.png"));
    Texture* specular_texture = CreateTexture(&ps->Textures,
                                              "SpecularTexture",
                                              path.Str(),
                                              {
                                                  .Type = ETextureType::Specular,
                                              });
    if (!specular_texture) {
        SDL_Log("ERROR: Loading specular texture");
        return false;
    }

    path = paths::PathJoin(scratch.Arena, ps->BasePath, String("assets/textures/matrix.jpg"));
    Texture* emissive_texture = CreateTexture(&ps->Textures,
                                              "EmissionTexture",
                                              path.Str(),
                                              {
                                                  .Type = ETextureType::Emissive,
                                                  .WrapT = GL_MIRRORED_REPEAT,
                                              });
    if (!emissive_texture) {
        SDL_Log("ERROR: Loading emissive texture");
        return false;
    }

    // Materials.

    {
        Material material{
            .TextureCount = 3,
            .Textures = {diffuse_texture, specular_texture, emissive_texture},
        };
        if (!CreateMaterial(&ps->Materials, "BoxMaterial", material)) {
            SDL_Log("ERROR: Creating box material");
            return false;
        }
    }

    // Models.

    {
        path = paths::PathJoin(scratch.Arena,
                               ps->BasePath,
                               String("assets/models/backpack/backpack.obj"));
        CreateModel(scratch.Arena, &ps->Models, "backpack", path.Str());
    }

    {
        path =
            paths::PathJoin(scratch.Arena, ps->BasePath, String("assets/models/sphere/scene.gltf"));
        CreateModel(scratch.Arena, &ps->Models, "sphere", path.Str());
    }

    {
        path = paths::PathJoin(scratch.Arena, ps->BasePath, String("assets/models/mini_dungeon"));
        if (auto files = paths::ListDir(scratch.Arena, path); IsValid(files)) {
            for (u32 i = 0; i < files.Size; i++) {
                paths::DirEntry& entry = files.Entries[i];
                if (!entry.IsFile()) {
                    continue;
                }

                SDL_Log("*** LOADING: %s\n", entry.Path.Str());

                auto basename = paths::GetBasename(scratch.Arena, entry.Path);
                Model* model = CreateModel(scratch.Arena,
                                           &ps->Models,
                                           basename.Str(),
                                           entry.Path.Str(),
                                           {.FlipUVs = true});
                if (model) {
                    ASSERT(gs->MiniDungeonModelCount < std::size(gs->MiniDungeonModels));
                    gs->MiniDungeonModels[gs->MiniDungeonModelCount++] = model;
                }
            }
        }
    }

    // Shaders.

    {
        String vs_path =
            paths::PathJoin(scratch.Arena, ps->BasePath, String("assets/shaders/shader.vert"));
        String fs_path =
            paths::PathJoin(scratch.Arena, ps->BasePath, String("assets/shaders/shader.frag"));
        if (!CreateShader(&ps->Shaders.Registry, "NormalShader", vs_path.Str(), fs_path.Str())) {
            return false;
        }
    }

    {
        String vs_path =
            paths::PathJoin(scratch.Arena, ps->BasePath, String("assets/shaders/light.vert"));
        String fs_path =
            paths::PathJoin(scratch.Arena, ps->BasePath, String("assets/shaders/light.frag"));
        if (!CreateShader(&ps->Shaders.Registry, "LightShader", vs_path.Str(), fs_path.Str())) {
            return false;
        }
    }

    {
        String vs_path = paths::PathJoin(scratch.Arena,
                                         ps->BasePath,
                                         String("assets/shaders/line_batcher.vert"));
        String fs_path = paths::PathJoin(scratch.Arena,
                                         ps->BasePath,
                                         String("assets/shaders/line_batcher.frag"));
        if (!CreateShader(&ps->Shaders.Registry,
                          "LineBatcherShader",
                          vs_path.Str(),
                          fs_path.Str())) {
            return false;
        }
    }

    glGenFramebuffers(1, &gs->DebugFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, gs->DebugFBO);

    glGenTextures(1, &gs->DebugFBOTexture);
    glBindTexture(GL_TEXTURE_2D, gs->DebugFBOTexture);
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
                           gs->DebugFBOTexture,
                           0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenRenderbuffers(1, &gs->DebugFBODepthStencil);
    glBindRenderbuffer(GL_RENDERBUFFER, gs->DebugFBODepthStencil);
    glRenderbufferStorage(GL_RENDERBUFFER,
                          GL_DEPTH24_STENCIL8,
                          ps->Window.Width,
                          ps->Window.Height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                              GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER,
                              gs->DebugFBODepthStencil);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    Init(&gs->EntityPicker);

    // Add the entities.

    {
        // Cubes
        for (const Vec3& position : kCubePositions) {
            auto [id, entity] = CreateEntity(&gs->EntityManager);
            gs->Boxes.Push(id);
            entity->Transform.Position = position;
        }

        for (u32 i = 0; i < kNumPointLights; i++) {
            Entity* owner = gs->PointLights[i]->GetOwner();
            owner->Transform = {};
            owner->Transform.Scale = Vec3(0.2f);
        }
    }

    return true;
}

bool GameUpdate(PlatformState* ps) {
    ImGuizmo::BeginFrame();
    ImGuizmo::Enable(true);
    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    GameState* gs = (GameState*)ps->GameState;
    ASSERT(gs);

    if (KEY_PRESSED(ps, SPACE)) {
        gs->MainCameraMode = !gs->MainCameraMode;
        SetupDebugCamera(gs->MainCamera, &gs->DebugCamera);
    }

    Update(ps, gs->CurrentCamera, ps->FrameDelta);
    Recalculate(&gs->MainCamera);
    Recalculate(&gs->DebugCamera);

    Recalculate(&gs->MainCamera);
    Recalculate(&gs->DebugCamera);
    if (gs->MainCameraMode) {
        Update(ps, &gs->MainCamera, ps->FrameDelta);
    } else {
        Update(ps, &gs->DebugCamera, ps->FrameDelta);
    }
    gs->CurrentCamera = gs->MainCameraMode ? &gs->MainCamera : &gs->DebugCamera;

    if (MOUSE_PRESSED(ps, LEFT)) {
        if (IsValid(gs->EntityManager, gs->HoverEntityID)) {
            gs->SelectedEntityID = gs->HoverEntityID;
        }
    }

    for (EntityID box : gs->Boxes) {
        auto* data = GetEntity(&gs->EntityManager, box);
        AddRotation(&data->Transform, Vec3(1.0f, 0.0f, 0.0f), 1.0f);
    }

    if (ImGui::Begin("Kandinsky")) {
        if (!gs->MainCameraMode) {
            ImGui::Text("DEBUG CAMERA");
        }
        ImGui::ColorEdit3("Clear Color", GetPtr(gs->ClearColor), ImGuiColorEditFlags_Float);

        if (ImGui::TreeNodeEx("Camera", ImGuiTreeNodeFlags_Framed)) {
            BuildImGui(&gs->MainCamera, gs->MainCameraMode ? NULL : gs->DebugFBOTexture);
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Debug Camera", ImGuiTreeNodeFlags_Framed)) {
            BuildImGui(&gs->DebugCamera);
            ImGui::TreePop();
        }

        ImGui::InputInt("Selected Entity",
                        &gs->SelectedEntityID.Value,
                        1,
                        100,
                        ImGuiInputTextFlags_ReadOnly);
        ImGui::InputInt("Hover Entity",
                        &gs->HoverEntityID.Value,
                        1,
                        100,
                        ImGuiInputTextFlags_ReadOnly);

        if (ImGui::CollapsingHeader("Input", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::InputFloat2("Mouse",
                               GetPtr(ps->InputState.MousePosition),
                               "%.3f",
                               ImGuiInputTextFlags_ReadOnly);
            ImGui::InputFloat2("Mouse (GL)",
                               GetPtr(ps->InputState.MousePositionGL),
                               "%.3f",
                               ImGuiInputTextFlags_ReadOnly);
        }

        /*
if (ImGui::TreeNodeEx("Lights",
                      ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
    if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        BuildImGui(&gs->EntityManager, gs->DirectionalLight);
    }

    for (u64 i = 0; i < std::size(gs->PointLights); i++) {
        String title = Printf(&ps->Memory.FrameArena, "Light %d", i);
        ImGui::PushID(title.Str());
        if (ImGui::CollapsingHeader(title.Str())) {
            BuildImGui(&gs->EntityManager, gs->PointLights[i]);
        }
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("Spotlight")) {
        BuildImGui(&gs->EntityManager, gs->Spotlight);
    }

    ImGui::TreePop();
}

if (IsValid(gs->EntityManager, gs->SelectedEntity)) {
    auto* entity_data = GetEntity(&gs->EntityManager, gs->SelectedEntity);
    if (entity_data->EntityType == EEntityType::PointLight) {
        PointLight* pl = &entity_data->PointLight;
        Transform& transform = entity_data->Transform;
        Debug::DrawSphere(ps, transform.Position, pl->MinRadius, 16, Color32::Black);
        Debug::DrawSphere(ps, transform.Position, pl->MaxRadius, 16, Color32::Grey);

        Mat4 model(1.0f);
        model = Translate(model, Vec3(transform.Position));
        if (ImGuizmo::Manipulate(GetPtr(gs->CurrentCamera->M_View),
                                 GetPtr(gs->CurrentCamera->M_Proj),
                                 ImGuizmo::TRANSLATE,
                                 ImGuizmo::WORLD,
                                 GetPtr(model))) {
            transform.Position = model[3];
        }
    }
}
        */
    }
    ImGui::End();

    return true;
}

// Render ------------------------------------------------------------------------------------------

namespace learn_opengl_private {

struct RenderSceneOptions {
    bool RenderDebugCamera = false;
};

void RenderScene(PlatformState* ps,
                 GameState* gs,
                 const Camera* camera,
                 const RenderSceneOptions options = {}) {
    Mesh* cube_mesh = FindMesh(&ps->Meshes, "Cube");
    ASSERT(cube_mesh);

    Shader* normal_shader = FindShader(&ps->Shaders.Registry, "NormalShader");
    ASSERT(normal_shader);
    Shader* light_shader = FindShader(&ps->Shaders.Registry, "LightShader");
    ASSERT(light_shader);
    Shader* line_batcher_shader = FindShader(&ps->Shaders.Registry, "LineBatcherShader");
    ASSERT(line_batcher_shader);

    Model* backpack_model = FindModel(&ps->Models, "backpack");
    ASSERT(backpack_model);

    Model* sphere_model = FindModel(&ps->Models, "sphere");
    ASSERT(sphere_model);

    Material* box_material = FindMaterial(&ps->Materials, "BoxMaterial");
    ASSERT(box_material);

    // Calculate the render state.
    RenderState rs = {};
    SetPlatformState(&rs, *ps);
    rs.Seconds = 0;
    // rs.Seconds = 0.5f * static_cast<float>(SDL_GetTicks()) / 1000.0f;
    SetCamera(&rs, *camera);

    static std::array<Light, 16> kLights = {};
    u32 light_count = 0;
    {
        kLights[light_count] = {.LightType = gs->DirectionalLight->StaticLightType(),
                                .DirectionalLight = *gs->DirectionalLight};
        light_count++;
    }

    for (auto* pl : gs->PointLights) {
        kLights[light_count] = {.LightType = pl->StaticLightType(), .PointLight = *pl};
        light_count++;
    }

    {
        kLights[light_count] = {.LightType = gs->Spotlight->StaticLightType(),
                                .Spotlight = *gs->Spotlight};
        light_count++;
    }

    Span<Light> lights(kLights.data(), light_count);
    SetLights(&rs, lights);

    /*
for (auto it = GetIteratorT<Box>(&gs->EntityManager); it; it++) {
    Box& box = *it;

    // Render cubes.
    Use(*normal_shader);

    // SetVec2(*normal_shader, "uMouseCoords", ps->InputState.MousePositionGL);
    // SetUVec2(*normal_shader, "uObjectID", box.Entity.EditorID.ToUVec2());
    SetEntity(&rs, box.Entity);

    ChangeModelMatrix(&rs, box.GetModelMatrix());
    Draw(*cube_mesh, *normal_shader, rs, box_material);
}
    */

    for (auto* pl : gs->PointLights) {
        Use(*light_shader);

        // SetVec2(*light_shader, "uMouseCoords", ps->InputState.MousePositionGL);
        // SetUVec2(*light_shader, "uObjectID", it->Entity.EditorID.ToUVec2());
        SetVec3(*light_shader, "uColor", Vec3(1.0f));
        SetEntity(&rs, pl->GetOwnerID());
        ChangeModelMatrix(&rs, pl->GetOwner()->M_Model);
        Draw(*cube_mesh, *light_shader, rs);
    }

    // Render model.
    {
        Use(*normal_shader);

        // Backpack.
        Mat4 mmodel = Mat4(1.0f);
        mmodel = Translate(mmodel, Vec3(2, 2, 2));
        ChangeModelMatrix(&rs, mmodel);
        Draw(*backpack_model, *normal_shader, rs);

        // Sphere.
        mmodel = Mat4(1.0f);
        mmodel = Translate(mmodel, Vec3(5, 5, 5));
        mmodel = Scale(mmodel, Vec3(0.1f));
        ChangeModelMatrix(&rs, mmodel);
        Draw(*sphere_model, *normal_shader, rs);

        u32 x = 0, z = 0;
        Vec3 offset(5, 0.1f, 0);
        for (u32 i = 0; i < gs->MiniDungeonModelCount; i++) {
            mmodel = Mat4(1.0f);
            mmodel = Translate(mmodel, offset + 2.0f * Vec3(x, 0, z));
            ChangeModelMatrix(&rs, mmodel);

            Draw(*gs->MiniDungeonModels[i], *normal_shader, rs);

            x++;
            if (x == 5) {
                x = 0;
                z++;
            }
        }
    }

    // Draw the camera.
    if (options.RenderDebugCamera) {
        Use(*light_shader);

        Mat4 mmodel(1.0f);
        mmodel = Translate(mmodel, gs->MainCamera.Position);
        mmodel = Scale(mmodel, Vec3(0.1f));
        ChangeModelMatrix(&rs, mmodel);

        Color32 color = Color32::MandarianOrange;
        SetVec3(*light_shader, "uColor", ToVec3(color));
        Draw(*cube_mesh, *light_shader, rs);

        Debug::DrawFrustum(ps, gs->MainCamera.M_ViewProj, color, 3);
    }

    Debug::Render(ps, *line_batcher_shader, camera->M_ViewProj);

    DrawGrid(rs);
}

}  // namespace learn_opengl_private

bool GameRender(PlatformState* ps) {
    using namespace learn_opengl_private;

    GameState* gs = (GameState*)ps->GameState;
    ASSERT(gs);

    UpdateModelMatrices(&gs->EntityManager);

    glViewport(0, 0, ps->Window.Width, ps->Window.Height);

    if (gs->MainCameraMode) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Render the main camera.
        glEnable(GL_DEPTH_TEST);
        glClearColor(gs->ClearColor.r, gs->ClearColor.g, gs->ClearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);

        StartFrame(&gs->EntityPicker);
        RenderScene(ps, gs, gs->CurrentCamera);
        gs->HoverEntityID = EndFrame(&gs->EntityPicker);

        ps->Functions.RenderImgui();
    } else {
        // DEBUG CAMERA MODE.

        glBindFramebuffer(GL_FRAMEBUFFER, gs->DebugFBO);

        // Render the main camera.
        glEnable(GL_DEPTH_TEST);
        glClearColor(gs->ClearColor.r, gs->ClearColor.g, gs->ClearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);

        RenderScene(ps, gs, &gs->MainCamera);

        // Now we render the scene from the debug camera POV.
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Render the main camera.
        glEnable(GL_DEPTH_TEST);
        glClearColor(gs->ClearColor.r, gs->ClearColor.g, gs->ClearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);

        RenderSceneOptions options = {};
        options.RenderDebugCamera = true;

        StartFrame(&gs->EntityPicker);
        RenderScene(ps, gs, &gs->DebugCamera, options);
        gs->HoverEntityID = EndFrame(&gs->EntityPicker);

        ps->Functions.RenderImgui();
    }

    return true;
}

}  // namespace kdk

#ifdef __cplusplus
}
#endif

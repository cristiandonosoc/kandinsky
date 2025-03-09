#include <learn_opengl/learn_opengl.h>

#include <kandinsky/debug.h>
#include <kandinsky/defines.h>
#include <kandinsky/glew.h>
#include <kandinsky/graphics/render_state.h>
#include <kandinsky/imgui.h>
#include <kandinsky/math.h>
#include <kandinsky/platform.h>
#include <kandinsky/print.h>
#include <kandinsky/utils/defer.h>
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
    gs->FreeCamera.Position = Vec3(-4.0f, 1.0f, 1.0f);
    gs->DirectionalLight.Direction = Vec3(-1.0f, -1.0f, -1.0f);
    gs->DirectionalLight.Color = {
        .Ambient = Vec3(0.05f),
        .Diffuse = Vec3(0.4f),
        .Specular = Vec3(0.05f),
    };
    for (u64 i = 0; i < std::size(gs->PointLights); i++) {
        PointLight& pl = gs->PointLights[i];
        pl.Color = {.Ambient = Vec3(0.05f), .Diffuse = Vec3(0.8f), .Specular = Vec3(1.0f)};
    }
    gs->PointLights[0].Position = Vec3(0.7f, 0.2f, 2.0f);
    gs->PointLights[1].Position = Vec3(2.3f, -3.3f, -4.0f);
    gs->PointLights[2].Position = Vec3(-4.0f, 2.0f, -12.0f);
    gs->PointLights[3].Position = Vec3(0.0f, 0.0f, -3.0f);

    gs->Spotlight.Position = Vec3(-1.0f);
    gs->Spotlight.Target = Vec3(0);
    gs->Spotlight.Color = {.Ambient = Vec3(0.05f), .Diffuse = Vec3(0.8f), .Specular = Vec3(1.0f)};

    gs->Material.Shininess = 10.0f;

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

    // // Meshes.
    // {
    //     CreateMeshOptions options{
    //         .Vertices = kVertices.data(),
    //         .Textures = {diffuse_texture, specular_texture, emissive_texture},
    //         .VertexCount = (u32)kVertices.size(),
    //         .TextureCount = 3,
    //     };

    //     if (!CreateMesh(&ps->Meshes, "CubeMesh", options)) {
    //         return false;
    //     }
    // }

    // if (!CreateMesh(&ps->Meshes,
    //                 "LightMesh",
    //                 {
    //                     .Vertices = kVertices.data(),
    //                     .VertexCount = (u32)kVertices.size(),
    //                 })) {
    //     return false;
    // }

    // Models.

    {
        path = paths::PathJoin(scratch.Arena,
                               ps->BasePath,
                               String("temp/models/backpack/backpack.obj"));
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
            for (u32 i = 0; i < files.Count; i++) {
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

    // Prepare SSBO.
    glGenBuffers(1, &gs->SSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gs->SSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 2 * sizeof(float), NULL, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, gs->SSBO);

    // Add the entities.

    {
        // Cubes
        for (const Vec3& position : kCubePositions) {
            Transform transform = {};
            transform.SetPosition(position);
            AddEntity(&gs->EntityManager, EEntityType::Box, transform);
        }

        if (Entity* dl = AddEntity(&gs->EntityManager, EEntityType::DirectionalLight)) {
            dl->Data = &gs->DirectionalLight;
        }

        for (u32 i = 0; i < kNumPointLights; i++) {
            PointLight& pl = gs->PointLights[i];

            Transform transform = {};
            transform.SetPosition(pl.Position);
            transform.SetScale(0.2f);

            Entity* entity = AddEntity(&gs->EntityManager, EEntityType::PointLight, transform);
            entity->Data = &gs->PointLights[i];
        }

        {
            Transform transform = {};
            transform.SetPosition(gs->Spotlight.Position);
            Entity* sl = AddEntity(&gs->EntityManager, EEntityType::Spotlight, transform);
            sl->Data = &gs->Spotlight;
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

    Update(ps, &gs->FreeCamera, ps->FrameDelta);

    if (MOUSE_PRESSED(ps, LEFT)) {
        if (gs->EntityManager.HoverEntityID != (u32)NONE) {
            gs->EntityManager.SelectedEntityID = gs->EntityManager.HoverEntityID;
        }
    }

    for (u32 i = 0; i < gs->EntityManager.EntityCount; i++) {
        Entity& entity = gs->EntityManager.Entities[i];
        if (entity.Type == EEntityType::Box) {
            Transform& transform = GetEntityTransform(&gs->EntityManager, entity);
            transform.AddRotation(Vec3(1.0f, 0.0f, 0.0f), 1.0f);
        }
    }

    if (ImGui::Begin("Kandinsky")) {
        ImGui::ColorEdit3("Clear Color", GetPtr(gs->ClearColor), ImGuiColorEditFlags_Float);

        ImGui::InputInt("Selected Entity",
                        (int*)&gs->EntityManager.SelectedEntityID,
                        1,
                        100,
                        ImGuiInputTextFlags_ReadOnly);

        ImGui::InputInt("Entity Index (hover)",
                        (int*)&gs->EntityManager.HoverEntityID,
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

        if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::InputFloat3("Position", GetPtr(gs->FreeCamera.Position));
        }

        ImGui::DragFloat("Shininess", &gs->Material.Shininess, 1.0f, 0.0f, 200);

        if (ImGui::TreeNodeEx("Lights",
                              ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
            if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                BuildImGui(&gs->DirectionalLight);
            }

            for (u64 i = 0; i < std::size(gs->PointLights); i++) {
                const char* title = Printf(&ps->Memory.FrameArena, "Light %d", i);
                ImGui::PushID(title);
                if (ImGui::CollapsingHeader(title)) {
                    PointLight& pl = gs->PointLights[i];
                    BuildImGui(&pl);
                }
                ImGui::PopID();
            }

            if (ImGui::CollapsingHeader("Spotlight")) {
                BuildImgui(&gs->Spotlight);
            }

            ImGui::TreePop();
        }

        if (gs->EntityManager.SelectedEntityID != (u32)NONE) {
            // Entity& entity = gs->EntityManager.Entities[gs->SelectedEntityID];

            if (Entity* entity = GetSelectedEntity(gs->EntityManager)) {
                if (entity->Type == EEntityType::PointLight) {
                    PointLight* pl = (PointLight*)entity->Data;

                    Debug::DrawSphere(ps, pl->Position, pl->MinRadius, 16, Color32::Black);
                    Debug::DrawSphere(ps, pl->Position, pl->MaxRadius, 16, Color32::Grey);

                    Mat4 model(1.0f);
                    model = Translate(model, Vec3(pl->Position));
                    if (ImGuizmo::Manipulate(GetPtr(gs->FreeCamera.View),
                                             GetPtr(gs->FreeCamera.Proj),
                                             ImGuizmo::TRANSLATE,
                                             ImGuizmo::WORLD,
                                             GetPtr(model))) {
                        pl->Position = model[3];
                    }
#if SPOTLIGHT
                    Spotlight& sl = gs->Spotlight;

                    Vec3 direction = GetDirection(sl);
                    Debug::DrawCone(ps,
                                    sl.Position,
                                    direction,
                                    sl.MaxCutoffDistance,
                                    ToRadians(sl.OuterRadiusDeg),
                                    16,
                                    Color32::Orange);

                    Mat4 posmat(1.0f);
                    posmat = Translate(posmat, Vec3(sl.Position));
                    if (ImGuizmo::Manipulate(GetPtr(gs->FreeCamera.View),
                                             GetPtr(gs->FreeCamera.Proj),
                                             ImGuizmo::TRANSLATE,
                                             ImGuizmo::WORLD,
                                             GetPtr(posmat))) {
                        sl.Position = posmat[3];
                        Recalculate(&sl);
                    }
#endif  // SPOTLIGHT
                }
            }
        }

        ImGui::End();
    }

    return true;
}

bool GameRender(PlatformState* ps) {
    using namespace kdk;

    GameState* gs = (GameState*)ps->GameState;
    ASSERT(gs);

    // LineBatcher* grid_line_batcher = FindLineBatcher(&ps->LineBatchers, "GridLineBatcher");
    // ASSERT(grid_line_batcher);

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

    // Texture* diffuse_texture = FindTexture(&ps->Textures, "DiffuseTexture");
    // ASSERT(diffuse_texture);
    // Texture* specular_texture = FindTexture(&ps->Textures, "SpecularTexture");
    // ASSERT(specular_texture);
    // Texture* emissive_texture = FindTexture(&ps->Textures, "EmissionTexture");
    // ASSERT(emissive_texture);

    // Calculate the render state.
    RenderState rs = {};
    rs.Seconds = 0;
    // rs.Seconds = 0.5f * static_cast<float>(SDL_GetTicks()) / 1000.0f;
    rs.CameraPosition = gs->FreeCamera.Position;
    rs.MatView = &gs->FreeCamera.View;
    rs.MatProj = &gs->FreeCamera.Proj;
    rs.MatViewProj = &gs->FreeCamera.ViewProj;
    rs.DirectionalLight.DL = &gs->DirectionalLight;
    rs.DirectionalLight.ViewDirection = (*rs.MatView) * Vec4(gs->DirectionalLight.Direction, 0.0f);
    for (u64 i = 0; i < std::size(gs->PointLights); i++) {
        rs.PointLights[i].PL = &gs->PointLights[i];
        rs.PointLights[i].ViewPosition = (*rs.MatView) * Vec4(gs->PointLights[i].Position, 1.0f);
    }
    rs.Spotlight.SL = &gs->Spotlight;
    rs.Spotlight.ViewPosition = (*rs.MatView) * Vec4(gs->Spotlight.Position, 1.0f);
    Vec3 spotlight_dir = gs->Spotlight.Target - gs->Spotlight.Position;
    rs.Spotlight.ViewDirection = (*rs.MatView) * Vec4(spotlight_dir, 0.0f);
    rs.Spotlight.InnerRadiusCos = Cos(ToRadians(gs->Spotlight.InnerRadiusDeg));
    rs.Spotlight.OuterRadiusCos = Cos(ToRadians(gs->Spotlight.OuterRadiusDeg));

    glViewport(0, 0, ps->Window.Width, ps->Window.Height);

    glEnable(GL_DEPTH_TEST);

    glClearColor(gs->ClearColor.r, gs->ClearColor.g, gs->ClearColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    // "Reset" the SSBO.

    {
        float values[2] = {
            std::numeric_limits<float>::max(),
            -1.0f,
        };
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(values), values);
    }

    DrawGrid(rs);

    const Mat4& mview = gs->FreeCamera.View;
    for (u32 entity_index = 0; entity_index < gs->EntityManager.EntityCount; entity_index++) {
        Entity& entity = gs->EntityManager.Entities[entity_index];
        if (!IsValid(entity)) {
            continue;
        }

        // Render cubes.
        if (entity.Type == EEntityType::Box) {
            Use(*normal_shader);

            SetFloat(*normal_shader, "uMaterial.Shininess", gs->Material.Shininess);
            // const auto& position = *(Vec3*)(entity.Ptr);

            const Mat4& mmodel = GetEntityModelMatrix(&gs->EntityManager, entity);

            Mat4 view_model = mview * mmodel;
            Mat4 normal_matrix = Transpose(Inverse(view_model));

            SetMat4(*normal_shader, "uViewModel", GetPtr(view_model));
            SetMat4(*normal_shader, "uNormalMatrix", GetPtr(normal_matrix));
            SetVec2(*normal_shader, "uMouseCoords", ps->InputState.MousePositionGL);
            SetFloat(*normal_shader, "uObjectID", (float)entity.ID);

            Draw(*cube_mesh, *normal_shader, rs);

            continue;
        }

        // Render Lights.
        if (entity.Type == EEntityType::PointLight) {
            Use(*light_shader);

            SetVec2(*light_shader, "uMouseCoords", ps->InputState.MousePositionGL);
            SetFloat(*light_shader, "uObjectID", (float)entity.ID);

            const Mat4& mmodel = GetEntityModelMatrix(&gs->EntityManager, entity);

            SetMat4(*light_shader, "uModel", GetPtr(mmodel));
            SetMat4(*light_shader, "uViewProj", GetPtr(*rs.MatViewProj));

            Draw(*cube_mesh, *light_shader, rs);
        }
    }

#if 0
    // Render model.
    {
        Use(*normal_shader);
        SetFloat(*normal_shader, "uMaterial.Shininess", gs->Material.Shininess);

        Mat4 mmodel = Mat4(1.0f);
        mmodel = Translate(mmodel, Vec3(2, 2, 2));

        Mat4 mview_model = mview * mmodel;
        Mat4 mnormal_matrix = Transpose(Inverse(mview_model));

        SetMat4(*normal_shader, "uViewModel", GetPtr(mview_model));
        SetMat4(*normal_shader, "uNormalMatrix", GetPtr(mnormal_matrix));

        Draw(*backpack_model, *normal_shader, rs);

        mmodel = Mat4(1.0f);
        mmodel = Translate(mmodel, Vec3(5, 5, 5));
        mmodel = Scale(mmodel, Vec3(0.1f));

        mview_model = mview * mmodel;
        mnormal_matrix = Transpose(Inverse(mview_model));

        SetMat4(*normal_shader, "uViewModel", GetPtr(mview_model));
        SetMat4(*normal_shader, "uNormalMatrix", GetPtr(mnormal_matrix));

        Draw(*sphere_model, *normal_shader, rs);

        u32 x = 0, z = 0;
        Vec3 offset(5, 0.1f, 0);
        for (u32 i = 0; i < gs->MiniDungeonModelCount; i++) {
            mmodel = Mat4(1.0f);
            mmodel = Translate(mmodel, offset + 2.0f * Vec3(x, 0, z));
            mview_model = mview * mmodel;
            mnormal_matrix = Transpose(Inverse(mview_model));

            SetMat4(*normal_shader, "uViewModel", GetPtr(mview_model));
            SetMat4(*normal_shader, "uNormalMatrix", GetPtr(mnormal_matrix));

            Draw(*gs->MiniDungeonModels[i], *normal_shader, rs);

            x++;
            if (x == 5) {
                x = 0;
                z++;
            }
        }
    }
#endif

    kdk::Debug::Render(ps, *line_batcher_shader, gs->FreeCamera.ViewProj);

    // Read the SSBO value.
    {
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        float values[2] = {};
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, gs->SSBO);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(values), values);

        gs->EntityManager.HoverEntityID = (u32)values[1];
    }

    return true;
}

}  // namespace kdk

#ifdef __cplusplus
}
#endif

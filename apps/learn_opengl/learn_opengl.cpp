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
    gs->Camera.Position = Vec3(-4.0f, 1.0f, 1.0f);
	gs->Camera.FreeCamera = {};

    float aspect_ratio = (float)(ps->Window.Width) / (float)(ps->Window.Height);
    SetProjection(&gs->Camera, Perspective(ToRadians(45.0f), aspect_ratio, 0.1f, 100.0f));

    gs->DirectionalLight.LightType = ELightType::Directional;
    gs->DirectionalLight.DirectionalLight.Direction = Vec3(-1.0f, -1.0f, -1.0f);
    gs->DirectionalLight.DirectionalLight.Color = {
        .Ambient = Vec3(0.05f),
        .Diffuse = Vec3(0.4f),
        .Specular = Vec3(0.05f),
    };
    for (u64 i = 0; i < std::size(gs->PointLights); i++) {
        Light& pl = gs->PointLights[i];
        pl.LightType = ELightType::Point;
        pl.PointLight.Color = {.Ambient = Vec3(0.05f),
                               .Diffuse = Vec3(0.8f),
                               .Specular = Vec3(1.0f)};
    }
    gs->PointLights[0].PointLight.Position = Vec3(0.7f, 0.2f, 2.0f);
    gs->PointLights[1].PointLight.Position = Vec3(2.3f, -3.3f, -4.0f);
    gs->PointLights[2].PointLight.Position = Vec3(-4.0f, 2.0f, -12.0f);
    gs->PointLights[3].PointLight.Position = Vec3(0.0f, 0.0f, -3.0f);

    gs->Spotlight.LightType = ELightType::Spotlight;
    gs->Spotlight.Spotlight.Position = Vec3(-1.0f);
    gs->Spotlight.Spotlight.Target = Vec3(0);
    gs->Spotlight.Spotlight.Color = {.Ambient = Vec3(0.05f),
                                     .Diffuse = Vec3(0.8f),
                                     .Specular = Vec3(1.0f)};

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

        if (Entity* dl = AddEntity(&gs->EntityManager, EEntityType::Light)) {
            dl->Data = &gs->DirectionalLight;
        }

        for (u32 i = 0; i < kNumPointLights; i++) {
            Light& pl = gs->PointLights[i];
            pl.LightType = ELightType::Point;

            Transform transform = {};
            transform.SetPosition(pl.PointLight.Position);
            transform.SetScale(0.2f);

            Entity* entity = AddEntity(&gs->EntityManager, EEntityType::Light, transform);
            entity->Data = &gs->PointLights[i];
        }

        {
            Transform transform = {};
            transform.SetPosition(gs->Spotlight.Spotlight.Position);
            Entity* sl = AddEntity(&gs->EntityManager, EEntityType::Light, transform);
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

    Update(ps, &gs->Camera, ps->FrameDelta);

    if (MOUSE_PRESSED(ps, LEFT)) {
        if (IsValid(gs->EntityManager.HoverEntityID)) {
            gs->EntityManager.SelectedEntityID = gs->EntityManager.HoverEntityID;
        }
    }

    if (auto* box_track = GetEntityTrack<EEntityType::Box>(&gs->EntityManager)) {
        for (u32 i = 0; i < box_track->EntityCount; i++) {
            Transform& transform = box_track->Transforms[i];
            transform.AddRotation(Vec3(1.0f, 0.0f, 0.0f), 1.0f);
        }
    }

    if (ImGui::Begin("Kandinsky")) {
        ImGui::ColorEdit3("Clear Color", GetPtr(gs->ClearColor), ImGuiColorEditFlags_Float);

        ImGui::Text("Selected Entity");
        ImGui::SameLine();
        BuildImgui(gs->EntityManager.SelectedEntityID);
        ImGui::Text("Hover Entity");
        ImGui::SameLine();
        BuildImgui(gs->EntityManager.HoverEntityID);

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
            ImGui::InputFloat3("Position", GetPtr(gs->Camera.Position));
        }

        if (ImGui::TreeNodeEx("Lights",
                              ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
            if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                BuildImGui(&gs->DirectionalLight.DirectionalLight);
            }

            for (u64 i = 0; i < std::size(gs->PointLights); i++) {
                const char* title = Printf(&ps->Memory.FrameArena, "Light %d", i);
                ImGui::PushID(title);
                if (ImGui::CollapsingHeader(title)) {
                    Light& pl = gs->PointLights[i];
                    BuildImGui(&pl.PointLight);
                }
                ImGui::PopID();
            }

            if (ImGui::CollapsingHeader("Spotlight")) {
                BuildImgui(&gs->Spotlight.Spotlight);
            }

            ImGui::TreePop();
        }

        if (IsValid(gs->EntityManager.SelectedEntityID)) {
            // Entity& entity = gs->EntityManager.Entities[gs->SelectedEntityID];

            if (Entity* entity = GetSelectedEntity(&gs->EntityManager)) {
                if (entity->GetEntityType() == EEntityType::Light) {
                    Transform& transform = GetEntityTransform(&gs->EntityManager, *entity);
                    Light* pl = (Light*)entity->Data;
                    if (pl->LightType == ELightType::Point) {
                        const Vec3& position = transform.GetPosition();
                        Debug::DrawSphere(ps,
                                          position,
                                          pl->PointLight.MinRadius,
                                          16,
                                          Color32::Black);
                        Debug::DrawSphere(ps,
                                          position,
                                          pl->PointLight.MaxRadius,
                                          16,
                                          Color32::Grey);

                        Mat4 model(1.0f);
                        model = Translate(model, Vec3(position));
                        if (ImGuizmo::Manipulate(GetPtr(gs->Camera.M_View),
                                                 GetPtr(gs->Camera.M_Proj),
                                                 ImGuizmo::TRANSLATE,
                                                 ImGuizmo::WORLD,
                                                 GetPtr(model))) {
                            pl->PointLight.Position = model[3];
                            transform.SetPosition(model[3]);
                        }
                    }
                }
            }
        }
    }
    ImGui::End();

    return true;
}

bool GameRender(PlatformState* ps) {
    using namespace kdk;

    GameState* gs = (GameState*)ps->GameState;
    ASSERT(gs);

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
    rs.Seconds = 0;
    // rs.Seconds = 0.5f * static_cast<float>(SDL_GetTicks()) / 1000.0f;
    SetCamera(&rs, gs->Camera);
    rs.DirectionalLight.DL = &gs->DirectionalLight.DirectionalLight;
    rs.DirectionalLight.ViewDirection = rs.M_View * Vec4(rs.DirectionalLight.DL->Direction, 0.0f);
    for (u64 i = 0; i < std::size(gs->PointLights); i++) {
        rs.PointLights[i].PL = &gs->PointLights[i].PointLight;
        rs.PointLights[i].ViewPosition = rs.M_View * Vec4(rs.PointLights[i].PL->Position, 1.0f);
    }
    rs.Spotlight.SL = &gs->Spotlight.Spotlight;
    rs.Spotlight.ViewPosition = rs.M_View * Vec4(gs->Spotlight.Spotlight.Position, 1.0f);
    Vec3 spotlight_dir = gs->Spotlight.Spotlight.Target - gs->Spotlight.Spotlight.Position;
    rs.Spotlight.ViewDirection = rs.M_View * Vec4(spotlight_dir, 0.0f);
    rs.Spotlight.InnerRadiusCos = Cos(ToRadians(gs->Spotlight.Spotlight.InnerRadiusDeg));
    rs.Spotlight.OuterRadiusCos = Cos(ToRadians(gs->Spotlight.Spotlight.OuterRadiusDeg));

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
            0,
        };
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(values), values);
    }

    DrawGrid(rs);

    if (auto* box_track = GetEntityTrack<EEntityType::Box>(&gs->EntityManager)) {
        for (u32 i = 0; i < box_track->EntityCount; i++) {
            Entity& entity = box_track->Entities[i];
            if (!IsValid(entity)) {
                continue;
            }

            // Render cubes.
            Use(*normal_shader);

            SetVec2(*normal_shader, "uMouseCoords", ps->InputState.MousePositionGL);
            SetU32(*normal_shader, "uObjectID", entity.ID.ID);

            ChangeModelMatrix(&rs, GetEntityModelMatrix(&gs->EntityManager, entity));
            Draw(*cube_mesh, *normal_shader, rs, box_material);
        }
    }

    if (auto* light_track = GetEntityTrack<EEntityType::Light>(&gs->EntityManager)) {
        for (u32 i = 0; i < light_track->EntityCount; i++) {
            Entity& entity = light_track->Entities[i];
            if (!IsValid(entity)) {
                continue;
            }

            Light* light = (Light*)entity.Data;
            if (light->LightType != ELightType::Point) {
                continue;
            }

            Use(*light_shader);

            SetVec2(*light_shader, "uMouseCoords", ps->InputState.MousePositionGL);
            SetU32(*light_shader, "uObjectID", entity.ID.ID);
            ChangeModelMatrix(&rs, GetEntityModelMatrix(&gs->EntityManager, entity));
            Draw(*cube_mesh, *light_shader, rs);
        }
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

    kdk::Debug::Render(ps, *line_batcher_shader, gs->Camera.M_ViewProj);

    // Read the SSBO value.
    {
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        u32 values[2] = {};
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, gs->SSBO);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(values), values);

        EntityID id = {};
        id.ID = (u32)values[1];
        gs->EntityManager.HoverEntityID = id;
    }

    return true;
}

}  // namespace kdk

#ifdef __cplusplus
}
#endif

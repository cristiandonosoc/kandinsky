#include <learn_opengl/learn_opengl.h>

#include <kandinsky/debug.h>
#include <kandinsky/defines.h>
#include <kandinsky/glew.h>
#include <kandinsky/graphics/model.h>
#include <kandinsky/graphics/render_state.h>
#include <kandinsky/imgui.h>
#include <kandinsky/input.h>
#include <kandinsky/math.h>
#include <kandinsky/platform.h>
#include <kandinsky/string.h>
#include <kandinsky/window.h>

#include <SDL3/SDL_mouse.h>

#include <imgui.h>

#include <string>

// #ifdef __cplusplus
// extern "C" {
// #endif

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

bool OnSharedObjectLoaded(PlatformState*) { return true; }

bool OnSharedObjectUnloaded(PlatformState*) { return true; }

bool GameInit(PlatformState* ps) {
    auto scratch = GetScratchArena();

    GameState* gs = (GameState*)ArenaPush(&ps->Memory.PermanentArena, sizeof(GameState));
    *gs = {};

    ps->MainCamera.Position = Vec3(-4.0f, 1.0f, 1.0f);

    {
        auto [id, entity] = CreateEntity(&ps->EntityManager,
                                         {
                                             .Name = String("DirectionalLight"),
                                         });
        auto [_, dl] = AddComponent<DirectionalLightComponent>(&ps->EntityManager, id);
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
            auto [id, entity] =
                CreateEntity(&ps->EntityManager,
                             {
                                 .Name = Printf(scratch.Arena, "PointLight_%llu", i),
                             });
            entity->Transform.Scale = Vec3(0.2f);

            auto [_, pl] = AddComponent<PointLightComponent>(&ps->EntityManager, id);
            gs->PointLights[i] = pl;

            pl->Color = {.Ambient = Vec3(0.05f), .Diffuse = Vec3(0.8f), .Specular = Vec3(1.0f)};
        }

        gs->PointLights[0]->GetOwner()->Transform.Position = Vec3(0.7f, 0.2f, 2.0f);
        gs->PointLights[1]->GetOwner()->Transform.Position = Vec3(2.3f, -3.3f, -4.0f);
        gs->PointLights[2]->GetOwner()->Transform.Position = Vec3(-4.0f, 2.0f, -12.0f);
        gs->PointLights[3]->GetOwner()->Transform.Position = Vec3(0.0f, 0.0f, -3.0f);
    }

    {
        auto [id, entity] = CreateEntity(&ps->EntityManager,
                                         {
                                             .Name = String("Spotlight"),
                                         });
        auto [_, sl] = AddComponent<SpotlightComponent>(&ps->EntityManager, entity->ID);
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
        Material material = {};
        material.Textures.Push(diffuse_texture);
        material.Textures.Push(specular_texture);
        material.Textures.Push(emissive_texture);
        if (gs->BoxMaterial = CreateMaterial(&ps->Materials, String("BoxMaterial"), material);
            !gs->BoxMaterial) {
            SDL_Log("ERROR: Creating box material");
            return false;
        }
    }

    // Models.

    path =
        paths::PathJoin(scratch.Arena, ps->BasePath, String("assets/models/backpack/backpack.obj"));
    if (gs->BackpackModel = CreateModel(scratch.Arena, &ps->Models, path); !gs->BackpackModel) {
        return false;
    }

    path = paths::PathJoin(scratch.Arena, ps->BasePath, String("assets/models/sphere/scene.gltf"));
    if (gs->SphereModel = CreateModel(scratch.Arena, &ps->Models, path); !gs->SphereModel) {
        return false;
    }

    path = paths::PathJoin(scratch.Arena, ps->BasePath, String("assets/models/mini_dungeon"));
    {
        auto files = paths::ListDir(scratch.Arena, path);
        for (u32 i = 0; i < files.size(); i++) {
            paths::DirEntry& entry = files[i];
            if (!entry.IsFile()) {
                continue;
            }

            SDL_Log("*** LOADING: %s\n", entry.Path.Str());

            Model* model = CreateModel(scratch.Arena, &ps->Models, entry.Path, {.FlipUVs = true});
            if (model) {
                ASSERT(gs->MiniDungeonModelCount < std::size(gs->MiniDungeonModels));
                gs->MiniDungeonModels[gs->MiniDungeonModelCount++] = model;
            }
        }
    }

    // Shaders.

    path = paths::PathJoin(scratch.Arena, ps->BasePath, String("assets/shaders/shader.glsl"));
    if (gs->NormalShader = CreateShader(&ps->Shaders, path); !gs->NormalShader) {
        return false;
    }

    path = paths::PathJoin(scratch.Arena, ps->BasePath, String("assets/shaders/light.glsl"));
    if (gs->LightShader = CreateShader(&ps->Shaders, path); !gs->LightShader) {
        return false;
    }

    path = paths::PathJoin(scratch.Arena, ps->BasePath, String("assets/shaders/line_batcher.glsl"));
    if (gs->LineBatcherShader = CreateShader(&ps->Shaders, path); !gs->LineBatcherShader) {
        return false;
    }


    // Add the entities.

    {
        // Cubes
        StaticModelComponent initial{
            .ModelPath = String("/Basic/Cube"),
        };

        for (u32 i = 0; i < std::size(kCubePositions); i++) {
            const Vec3& position = kCubePositions[i];
            auto [id, entity] = CreateEntity(&ps->EntityManager,
                                             {
                                                 .Name = Printf(scratch.Arena, "Cube_%d", i),
                                             });
            gs->Boxes.Push(id);
            entity->Transform.Position = position;
            AddComponent<StaticModelComponent>(&ps->EntityManager, id, &initial);
        }
    }

    return true;
}

bool GameUpdate(PlatformState* ps) {
    GameState* gs = (GameState*)ps->GameState;
    ASSERT(gs);

    if (KEY_PRESSED(ps, SPACE)) {
        ps->MainCameraMode = !ps->MainCameraMode;
        SetupDebugCamera(ps->MainCamera, &ps->DebugCamera);
    }

    Update(ps, ps->CurrentCamera, ps->FrameDelta);
    Recalculate(&ps->MainCamera);
    Recalculate(&ps->DebugCamera);

    Recalculate(&ps->MainCamera);
    Recalculate(&ps->DebugCamera);
    if (ps->MainCameraMode) {
        Update(ps, &ps->MainCamera, ps->FrameDelta);
    } else {
        Update(ps, &ps->DebugCamera, ps->FrameDelta);
    }
    ps->CurrentCamera = ps->MainCameraMode ? &ps->MainCamera : &ps->DebugCamera;

    if (MOUSE_PRESSED(ps, LEFT)) {
        if (IsValid(ps->EntityManager, ps->HoverEntityID)) {
            ps->SelectedEntityID = ps->HoverEntityID;
        }
    }

    for (EntityID box : gs->Boxes) {
        auto* data = GetEntity(&ps->EntityManager, box);
        AddRotation(&data->Transform, Vec3(1.0f, 0.0f, 0.0f), 1.0f);
    }

    if (ImGui::Begin("Kandinsky")) {
        auto scratch = GetScratchArena();

        if (!ps->MainCameraMode) {
            ImGui::Text("DEBUG CAMERA");
        }
        ImGui::ColorEdit3("Clear Color", GetPtr(ps->ClearColor), ImGuiColorEditFlags_Float);

        if (ImGui::TreeNodeEx("Camera", ImGuiTreeNodeFlags_Framed)) {
            BuildImGui(&ps->MainCamera, ps->MainCameraMode ? NULL : ps->DebugFBOTexture);
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Debug Camera", ImGuiTreeNodeFlags_Framed)) {
            BuildImGui(&ps->DebugCamera);
            ImGui::TreePop();
        }
        if (Entity* entity = GetEntity(&ps->EntityManager, ps->HoverEntityID)) {
            ImGui::Text("Hover: %d (Index: %d, Gen: %d) - Type: %s\n",
                        ps->HoverEntityID.Value,
                        entity->ID.GetIndex(),
                        entity->ID.GetGeneration(),
                        ToString(entity->EntityType));
        } else {
            ImGui::Text("Hover Entity: NONE");
        }

        ImGui::Separator();

        if (Entity* entity = GetEntity(&ps->EntityManager, ps->SelectedEntityID)) {
            String label = Printf(scratch.Arena,
                                  "Selected: %d (Index: %d, Gen: %d) - Type: %s\n",
                                  ps->SelectedEntityID.Value,
                                  ps->SelectedEntityID.GetIndex(),
                                  ps->SelectedEntityID.GetGeneration(),
                                  ToString(entity->EntityType));

            if (ImGui::TreeNodeEx(label.Str(),
                                  ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) {
                BuildImGui(&ps->EntityManager, ps->SelectedEntityID);
                ImGui::TreePop();
            }

            BuildGizmos(ps, *ps->CurrentCamera, &ps->EntityManager, ps->SelectedEntityID);
        }

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

        // if (ImGui::TreeNodeEx("Lights",
        //                       ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
        //     if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        //         BuildImGui(&ps->EntityManager, gs->DirectionalLight);
        //     }
        //
        //     for (u64 i = 0; i < std::size(gs->PointLights); i++) {
        //         String title = Printf(&ps->Memory.FrameArena, "Light %d", i);
        //         ImGui::PushID(title.Str());
        //         if (ImGui::CollapsingHeader(title.Str())) {
        //             BuildImGui(&ps->EntityManager, gs->PointLights[i]);
        //         }
        //         ImGui::PopID();
        //     }
        //
        //     if (ImGui::CollapsingHeader("Spotlight")) {
        //         BuildImGui(&ps->EntityManager, gs->Spotlight);
        //     }
        //
        //     ImGui::TreePop();
        // }
    }
    ImGui::End();

    return true;
}

// Render ------------------------------------------------------------------------------------------

namespace learn_opengl_private {

void RenderScene(PlatformState* ps,
                 GameState* gs,
                 const Camera* camera,
                 const RenderSceneOptions options = {}) {
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

    // Render Boxes.
    Use(*gs->NormalShader);
    for (EntityID box_id : gs->Boxes) {
        Entity* entity = GetEntity(&ps->EntityManager, box_id);
        ASSERT(entity);
        SetVec3(*gs->NormalShader, "uColor", Vec3(1.0f));
        SetEntity(&rs, box_id);
        ChangeModelMatrix(&rs, entity->M_Model);
        Draw(*ps->BaseAssets.CubeModel, *gs->NormalShader, rs);
    }

    for (auto* pl : gs->PointLights) {
        // SetVec2(*gs->LightShader, "uMouseCoords", ps->InputState.MousePositionGL);
        // SetUVec2(*gs->LightShader, "uObjectID", it->Entity.EditorID.ToUVec2());
        SetVec3(*gs->LightShader, "uColor", Vec3(1.0f));
        SetEntity(&rs, pl->GetOwnerID());
        ChangeModelMatrix(&rs, pl->GetOwner()->M_Model);
        Draw(*ps->BaseAssets.CubeModel, *gs->LightShader, rs);
    }

    SetEntity(&rs, {});

    // Render model.
    {
        // Backpack.
        Mat4 mmodel = Mat4(1.0f);
        mmodel = Translate(mmodel, Vec3(2, 2, 2));
        ChangeModelMatrix(&rs, mmodel);
        Draw(*gs->BackpackModel, *gs->NormalShader, rs);

        // Sphere.
        mmodel = Mat4(1.0f);
        mmodel = Translate(mmodel, Vec3(5, 5, 5));
        mmodel = Scale(mmodel, Vec3(0.1f));
        ChangeModelMatrix(&rs, mmodel);
        Draw(*gs->SphereModel, *gs->NormalShader, rs);

        u32 x = 0, z = 0;
        Vec3 offset(5, 0.1f, 0);
        for (u32 i = 0; i < gs->MiniDungeonModelCount; i++) {
            mmodel = Mat4(1.0f);
            mmodel = Translate(mmodel, offset + 2.0f * Vec3(x, 0, z));
            ChangeModelMatrix(&rs, mmodel);

            Draw(*gs->MiniDungeonModels[i], *gs->NormalShader, rs);

            x++;
            if (x == 5) {
                x = 0;
                z++;
            }
        }
    }

    // Draw the camera.
    if (options.RenderDebugCamera) {
        Use(*gs->LightShader);

        Mat4 mmodel(1.0f);
        mmodel = Translate(mmodel, ps->MainCamera.Position);
        mmodel = Scale(mmodel, Vec3(0.1f));
        ChangeModelMatrix(&rs, mmodel);

        Color32 color = Color32::MandarianOrange;
        SetVec3(*gs->LightShader, "uColor", ToVec3(color));
        Draw(*ps->BaseAssets.CubeModel, *gs->LightShader, rs);

        Debug::DrawFrustum(ps, ps->MainCamera.M_ViewProj, color, 3);
    }

    Debug::Render(ps, *gs->LineBatcherShader, camera->M_ViewProj);

    DrawGrid(rs);
}

}  // namespace learn_opengl_private

bool GameRender(PlatformState* ps) {
    using namespace learn_opengl_private;

    GameState* gs = (GameState*)ps->GameState;
    ASSERT(gs);

    RenderScene(ps, gs, ps->CurrentCamera, ps->RenderSceneOptions);

    return true;
}

}  // namespace kdk

// #ifdef __cplusplus
// }
// #endif

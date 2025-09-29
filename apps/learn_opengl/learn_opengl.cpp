#include <learn_opengl/learn_opengl.h>

#include <kandinsky/core/defines.h>
#include <kandinsky/core/math.h>
#include <kandinsky/core/string.h>
#include <kandinsky/debug.h>
#include <kandinsky/glew.h>
#include <kandinsky/graphics/model.h>
#include <kandinsky/graphics/render_state.h>
#include <kandinsky/graphics/texture.h>
#include <kandinsky/imgui.h>
#include <kandinsky/input.h>
#include <kandinsky/platform.h>
#include <kandinsky/window.h>

#include <SDL3/SDL_mouse.h>

#include <imgui.h>

#include <string>

#define CREATE_HARDCODED_ENTITIES 1

namespace kdk {

// clang-format off
Vec3 kCubePositions[] = {Vec3( 5.0f, 0.0f,  5.0f),
						 Vec3(-5.0f, 0.0f,  5.0f),
						 Vec3( 5.0f, 0.0f, -5.0f),
						 Vec3(-5.0f, 0.0f, -5.0f)};
// clang-format on

bool OnSharedObjectLoaded(PlatformState*) { return true; }

bool OnSharedObjectUnloaded(PlatformState*) { return true; }

bool GameInit(PlatformState* ps) {
    (void)ps;
    auto scratch = GetScratchArena();

    GameState* gs = (GameState*)ArenaPush(&ps->Memory.PermanentArena, sizeof(GameState));
    *gs = {};

#if CREATE_HARDCODED_ENTITIES

    ps->MainCamera.Position = Vec3(-4.0f, 1.0f, 1.0f);

    {
        auto [id, entity] = CreateEntityOpaque(ps->EntityManager,
                                               EEntityType::Test,
                                               {
                                                   .Name = String("DirectionalLight"),
                                               });
        auto [_, dl] = AddComponent<DirectionalLightComponent>(ps->EntityManager, id);
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
                CreateEntityOpaque(ps->EntityManager,
                                   EEntityType::Test,
                                   {
                                       .Name = Printf(scratch.Arena, "PointLight_%llu", i),
                                   });
            entity->Transform.Scale = Vec3(0.2f);

            auto [_, pl] = AddComponent<PointLightComponent>(ps->EntityManager, id);
            gs->PointLights[i] = pl;

            pl->Color = {.Ambient = Vec3(0.05f), .Diffuse = Vec3(0.8f), .Specular = Vec3(1.0f)};
        }

        gs->PointLights[0]->GetOwner()->Transform.Position = Vec3(0.7f, 0.2f, 2.0f);
        gs->PointLights[1]->GetOwner()->Transform.Position = Vec3(2.3f, -3.3f, -4.0f);
        gs->PointLights[2]->GetOwner()->Transform.Position = Vec3(-4.0f, 2.0f, -12.0f);
        gs->PointLights[3]->GetOwner()->Transform.Position = Vec3(0.0f, 0.0f, -3.0f);
    }

    {
        auto [id, entity] = CreateEntityOpaque(ps->EntityManager,
                                               EEntityType::Test,
                                               {
                                                   .Name = String("Spotlight"),
                                               });
        auto [_, sl] = AddComponent<SpotlightComponent>(ps->EntityManager, entity->ID);
        gs->Spotlight = sl;

        entity->Transform.Position = Vec3(-1.0f);
        sl->Target = Vec3(0);
        sl->Color = {.Ambient = Vec3(0.05f), .Diffuse = Vec3(0.8f), .Specular = Vec3(1.0f)};
    }

    ps->GameState = gs;

    TextureAssetHandle diffuse_texture = CreateTexture(&ps->Assets,
                                                       String("textures/container2.png"),
                                                       {
                                                           .Type = ETextureType::Diffuse,

                                                       });
    if (!IsValid(diffuse_texture)) {
        SDL_Log("ERROR: Loading diffuse texture");
        return false;
    }

    TextureAssetHandle specular_texture = CreateTexture(&ps->Assets,
                                                        String("textures/container2_specular.png"),
                                                        {
                                                            .Type = ETextureType::Specular,
                                                        });
    if (!IsValid(specular_texture)) {
        SDL_Log("ERROR: Loading specular texture");
        return false;
    }

    TextureAssetHandle emissive_texture = CreateTexture(&ps->Assets,
                                                        String("textures/matrix.jpg"),
                                                        {
                                                            .Type = ETextureType::Emissive,
                                                            .WrapT = GL_MIRRORED_REPEAT,
                                                        });
    if (!IsValid(emissive_texture)) {
        SDL_Log("ERROR: Loading emissive texture");
        return false;
    }

    // Materials.

    {
        CreateMaterialParams params = {};
        params.TextureHandles.Push(diffuse_texture);
        params.TextureHandles.Push(specular_texture);
        params.TextureHandles.Push(emissive_texture);
        if (gs->BoxMaterialHandle = CreateMaterial(&ps->Assets, String("BoxMaterial"sv), params);
            !IsValid(gs->BoxMaterialHandle)) {
            SDL_Log("ERROR: Creating box material");
            return false;
        }
    }

    // Models.

    if (gs->BackpackModelHandle =
            CreateModel(&ps->Assets, String("models/backpack/backpack.obj"sv));
        !IsValid(gs->BackpackModelHandle)) {
        return false;
    }

    String path =
        paths::PathJoin(scratch.Arena, ps->Assets.AssetBasePath, String("models/mini_dungeon"sv));
    {
        auto files = paths::ListDir(scratch.Arena, path);
        for (u32 i = 0; i < files.size(); i++) {
            paths::DirEntry& entry = files[i];
            if (!entry.IsFile()) {
                continue;
            }

            String asset_path = RemovePrefix(scratch, entry.Path, ps->Assets.AssetBasePath);
            SDL_Log("*** LOADING: %s\n", asset_path.Str());

            if (ModelAssetHandle handle = CreateModel(&ps->Assets, asset_path, {.FlipUVs = true});
                IsValid(handle)) {
                gs->MiniDungeonModelHandles.Push(handle);
            }
        }
    }

    StaticModelComponent initial_model{};

    // Add the entities.
    // Cubes
    {
        initial_model.ModelHandle = ps->Assets.BaseAssets.CubeModelHandle;

        for (u32 i = 0; i < std::size(kCubePositions); i++) {
            const Vec3& position = kCubePositions[i];
            auto [id, entity] = CreateEntityOpaque(ps->EntityManager,
                                                   EEntityType::Test,
                                                   {
                                                       .Name = Printf(scratch.Arena, "Cube_%d", i),
                                                   });
            gs->Boxes.Push(id);
            entity->Transform.Position = position;
            AddComponent<StaticModelComponent>(ps->EntityManager, id, &initial_model);
        }
    }

    // Sphere.
    {
        initial_model.ModelHandle = ps->Assets.BaseAssets.SphereModelHandle;

        auto [id, entity] = CreateEntityOpaque(ps->EntityManager,
                                               EEntityType::Test,
                                               {
                                                   .Name = String("Sphere"),
                                               });
        entity->Transform.Position = Vec3(5, 5, 5);
        entity->Transform.Scale = Vec3(0.1f);
        AddComponent<StaticModelComponent>(ps->EntityManager, id, &initial_model);
    }

    // Backpack.
    {
        initial_model.ModelHandle = gs->BackpackModelHandle;

        auto [id, entity] = CreateEntityOpaque(ps->EntityManager,
                                               EEntityType::Test,
                                               {
                                                   .Name = String("Backpack"),
                                               });
        entity->Transform.Position = Vec3(2, 2, 2);
        AddComponent<StaticModelComponent>(ps->EntityManager, id, &initial_model);
    }

    // Mini dungeon.
    {
        u32 x = 0, z = 0;
        Vec3 offset(5, 0.1f, 0);
        for (i32 i = 0; i < gs->MiniDungeonModelHandles.Size; i++) {
            initial_model.ModelHandle = gs->MiniDungeonModelHandles[i];

            auto [id, entity] =
                CreateEntityOpaque(ps->EntityManager,
                                   EEntityType::Test,
                                   {
                                       .Name = Printf(scratch, "MiniDungeonModel_%d", i),
                                   });
            entity->Transform.Position = offset + 2.0f * Vec3(x, 0, z);
            AddComponent<StaticModelComponent>(ps->EntityManager, id, &initial_model);

            x++;
            if (x == 5) {
                x = 0;
                z++;
            }
        }
    }

#endif  // CREATE_HARDCODED_ENTITIES

    return true;
}

bool GameUpdate(PlatformState* ps) {
    GameState* gs = (GameState*)ps->GameState;
    ASSERT(gs);

    for (EntityID box : gs->Boxes) {
        if (Entity* data = GetEntity(ps->EntityManager, box)) {
            AddRotation(&data->Transform,
                        Vec3(1.0f, 0.0f, 0.0f),
                        (float)(25.0f * ps->CurrentTimeTracking->DeltaSeconds));
        }
    }

    return true;
}

// Render ------------------------------------------------------------------------------------------

bool GameRender(PlatformState*) { return true; }

}  // namespace kdk

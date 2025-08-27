#include <kandinsky/asset_registry.h>

#include <kandinsky/platform.h>
#include "kandinsky/core/string.h"

namespace kdk {

// BASE ASSETS -------------------------------------------------------------------------------------

namespace asset_registry_private {

bool LoadInitialMaterials(AssetRegistry* assets) {
    // We create a fake white material.

    CreateMaterialOptions white_material_options{
        .Albedo = ToVec3(Color32::White),
    };
    assets->BaseAssets.WhiteMaterialHandle =
        CreateMaterial(assets, String("/Basic/Materials/White"), white_material_options);

    return true;
}

bool LoadInitialShaders(PlatformState* ps, AssetRegistry* assets) {
    auto scratch = GetScratchArena();

    String path;

    if (assets->BaseAssets.NormalShaderHandle =
            CreateShader(&ps->Assets, String("shaders/shader.glsl"));
        !IsValid(assets->BaseAssets.NormalShaderHandle)) {
        SDL_Log("ERROR: Creating base shader from %s", path.Str());
        return false;
    }

    if (assets->BaseAssets.LightShaderHandle =
            CreateShader(&ps->Assets, String("shaders/light.glsl"));
        !IsValid(assets->BaseAssets.LightShaderHandle)) {
        SDL_Log("ERROR: Creating base shader from %s", path.Str());
        return false;
    }

    if (assets->BaseAssets.LineBatcherShaderHandle =
            CreateShader(&ps->Assets, String("shaders/line_batcher.glsl"));
        !IsValid(assets->BaseAssets.LineBatcherShaderHandle)) {
        SDL_Log("ERROR: Creating base shader from %s", path.Str());
        return false;
    }

    if (assets->BaseAssets.GridShaderHandle =
            CreateShader(&ps->Assets, String("shaders/grid.glsl"));
        !IsValid(assets->BaseAssets.GridShaderHandle)) {
        SDL_Log("ERROR: Creating base shader from %s", path.Str());
        return false;
    }
    glGenVertexArrays(1, &assets->BaseAssets.GridVAO);

    return true;
}

// clang-format off
std::array kCubeVertices = {
    // positions          // normals           // texture coords
	Vertex{{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f,},  {0.0f, 0.0f}},
    Vertex{{ 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f,},  {1.0f, 0.0f}},
    Vertex{{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f,},  {1.0f, 1.0f}},
    Vertex{{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f,},  {1.0f, 1.0f}},
    Vertex{{-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f,},  {0.0f, 1.0f}},
    Vertex{{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f,},  {0.0f, 0.0f}},

    Vertex{{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f,},  {0.0f, 0.0f}},
    Vertex{{ 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f,},  {1.0f, 0.0f}},
    Vertex{{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f,},  {1.0f, 1.0f}},
    Vertex{{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f,},  {1.0f, 1.0f}},
    Vertex{{-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f,},  {0.0f, 1.0f}},
    Vertex{{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f,},  {0.0f, 0.0f}},

    Vertex{{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f,},  {1.0f, 0.0f}},
    Vertex{{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f,},  {1.0f, 1.0f}},
    Vertex{{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f,},  {0.0f, 1.0f}},
    Vertex{{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f,},  {0.0f, 1.0f}},
    Vertex{{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f,},  {0.0f, 0.0f}},
    Vertex{{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f,},  {1.0f, 0.0f}},

    Vertex{{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f,},  {1.0f, 0.0f}},
    Vertex{{ 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f,},  {1.0f, 1.0f}},
    Vertex{{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f,},  {0.0f, 1.0f}},
    Vertex{{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f,},  {0.0f, 1.0f}},
    Vertex{{ 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f,},  {0.0f, 0.0f}},
    Vertex{{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f,},  {1.0f, 0.0f}},

    Vertex{{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f,},  {0.0f, 1.0f}},
    Vertex{{ 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f,},  {1.0f, 1.0f}},
    Vertex{{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f,},  {1.0f, 0.0f}},
    Vertex{{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f,},  {1.0f, 0.0f}},
    Vertex{{-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f,},  {0.0f, 0.0f}},
    Vertex{{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f,},  {0.0f, 1.0f}},

    Vertex{{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f,},  {0.0f, 1.0f}},
    Vertex{{ 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f,},  {1.0f, 1.0f}},
    Vertex{{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f,},  {1.0f, 0.0f}},
    Vertex{{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f,},  {1.0f, 0.0f}},
    Vertex{{-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f,},  {0.0f, 0.0f}},
    Vertex{{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f,},  {0.0f, 1.0f}},
};
// clang-format on

bool LoadInitialMeshes(AssetRegistry* assets) {
    auto scratch = GetScratchArena();

    // Cube.
    {
        CreateMeshOptions options{
            .Vertices = kCubeVertices,
        };
        MeshAssetHandle cube_mesh_handle = CreateMesh(assets, String("Cube"sv), options);
        if (!IsValid(cube_mesh_handle)) {
            SDL_Log("ERROR: Creating cube mesh");
            return false;
        }

        ModelMeshBinding cube_material_binding{
            .MeshHandle = cube_mesh_handle,
            .MaterialHandle = assets->BaseAssets.WhiteMaterialHandle,
        };

        if (assets->BaseAssets.CubeModelHandle =
                CreateSyntheticModel(assets,
                                     String("/Basic/Cube"),
                                     MakeSpan(cube_material_binding));
            !IsValid(assets->BaseAssets.CubeModelHandle)) {
            SDL_Log("ERROR: Creating cube model from mesh");
            return false;
        }
    }

    // Sphere.
    {
        if (assets->BaseAssets.SphereModelHandle =
                CreateModel(assets, String("models/sphere/scene.gltf"));
            !IsValid(assets->BaseAssets.SphereModelHandle)) {
            SDL_Log("ERROR: Creating sphere mesh");
            return false;
        }
    }

    return true;
}

bool LoadBaseAssets(PlatformState* ps, AssetRegistry* assets) {
    using namespace asset_registry_private;
    if (!LoadInitialMaterials(assets)) {
        SDL_Log("ERROR: Loading initial materials");
        return false;
    }

    if (!LoadInitialShaders(ps, assets)) {
        SDL_Log("ERROR: Loading initial shaders");
        return false;
    }

    if (!LoadInitialMeshes(assets)) {
        SDL_Log("ERROR: Loading initial meshes");
        return false;
    }

    return true;
}

template <typename T, u32 SIZE>
std::pair<Asset*, T*> FindAsset(AssetHolder<T, SIZE>* asset_holder, i32 id) {
    for (i32 i = 0; i < asset_holder->Assets.Size; i++) {
        auto& asset = asset_holder->Assets[i];
        if (asset.AssetID == id) {
            return {i, &asset};
        }
    }
    return {NONE, nullptr};
}

}  // namespace asset_registry_private

bool Init(PlatformState* ps, AssetRegistry* assets) {
    assets->AssetBasePath =
        paths::PathJoin(ps->Memory.StringArena.GetPtr(), ps->BasePath, String("assets"sv));
    assets->AssetLoadingArena = ps->Memory.AssetLoadingArena.GetPtr();

    if (!asset_registry_private::LoadBaseAssets(ps, assets)) {
        SDL_Log("ERROR: Loading base assets");
        return false;
    }
    return true;
}

void Shutdown(PlatformState* ps, AssetRegistry* assets) {
    (void)ps;
    assets->AssetBasePath = {};
    assets->AssetLoadingArena = nullptr;
}

String GetFullAssetPath(Arena* arena, AssetRegistry* assets, String asset_path) {
    return paths::PathJoin(arena, assets->AssetBasePath, asset_path);
}

AssetHandle FindAssetHandle(AssetRegistry* assets, EAssetType asset_type, String asset_path) {
    i32 asset_id = GenerateAssetID(asset_type, asset_path);

#define X(enum_name, struct_name, ...)                                \
    case EAssetType::enum_name: {                                     \
        return assets->struct_name##Holder.FindAssetHandle(asset_id); \
    }

    switch (asset_type) {
        ASSET_TYPES(X)
        case EAssetType::Invalid: ASSERT(false); return {};
        case EAssetType::COUNT: ASSERT(false); return {};
    }
#undef X

    ASSERT(false);
    return {};
}

std::pair<Asset*, void*> FindAsset(AssetRegistry* assets, AssetHandle handle) {
#define X(enum_name, struct_name, ...)                        \
    case EAssetType::enum_name: {                             \
        return assets->struct_name##Holder.FindAsset(handle); \
    }

    switch (handle.GetAssetType()) {
        ASSET_TYPES(X)
        case EAssetType::Invalid: ASSERT(false); return {};
        case EAssetType::COUNT: ASSERT(false); return {};
    }
#undef X

    ASSERT(false);
    return {nullptr, nullptr};
}

}  // namespace kdk

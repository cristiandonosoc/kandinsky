#include <kandinsky/asset_registry.h>

#include <kandinsky/core/file.h>
#include <kandinsky/core/string.h>
#include <kandinsky/platform.h>

namespace kdk {

// BASE ASSETS -------------------------------------------------------------------------------------

namespace asset_registry_private {

bool LoadInitialMaterials(AssetRegistry* assets) {
    // We create a fake white material.

    CreateMaterialParams white_material_params{
        .AssetOptions = {.IsBaseAsset = true},
        .Albedo = ToVec3(Color32::White),
    };
    assets->BaseAssets.WhiteMaterialHandle =
        CreateMaterial(assets, String("/Basic/Materials/White"), white_material_params);

    return true;
}

bool LoadInitialShaders(PlatformState* ps, AssetRegistry* assets) {
    auto scratch = GetScratchArena();

    String path;

    CreateShaderParams shader_params{
        .AssetOptions = {.IsBaseAsset = true},
    };

    if (assets->BaseAssets.NormalShaderHandle =
            CreateShader(&ps->Assets, String("shaders/shader.glsl"), shader_params);
        !IsValid(assets->BaseAssets.NormalShaderHandle)) {
        SDL_Log("ERROR: Creating base shader from %s", path.Str());
        return false;
    }

    if (assets->BaseAssets.LightShaderHandle =
            CreateShader(&ps->Assets, String("shaders/light.glsl"), shader_params);
        !IsValid(assets->BaseAssets.LightShaderHandle)) {
        SDL_Log("ERROR: Creating base shader from %s", path.Str());
        return false;
    }

    if (assets->BaseAssets.LineBatcherShaderHandle =
            CreateShader(&ps->Assets, String("shaders/line_batcher.glsl"), shader_params);
        !IsValid(assets->BaseAssets.LineBatcherShaderHandle)) {
        SDL_Log("ERROR: Creating base shader from %s", path.Str());
        return false;
    }

    if (assets->BaseAssets.GridShaderHandle =
            CreateShader(&ps->Assets, String("shaders/grid.glsl"), shader_params);
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

    CreateModelParams model_params{
        .AssetOptions = {.IsBaseAsset = true},
    };

    // Cube.
    {
        CreateMeshParams params{
            .AssetOptions = {.IsBaseAsset = true},
            .Vertices = kCubeVertices,
        };
        MeshAssetHandle cube_mesh_handle = CreateMesh(assets, String("Cube"sv), params);
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
                                     MakeSpan(cube_material_binding),
                                     model_params);
            !IsValid(assets->BaseAssets.CubeModelHandle)) {
            SDL_Log("ERROR: Creating cube model from mesh");
            return false;
        }
    }

    // Sphere.
    {
        if (assets->BaseAssets.SphereModelHandle =
                CreateModel(assets, String("models/sphere/scene.gltf"), model_params);
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

i32 GenerateAssetID(EAssetType type, String asset_path) {
    return IDFromString(asset_path.Str()) + (i32)type;
}

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

#define X(enum_name, ...) Create##enum_name##Params enum_name##Params;
struct AssetParams {
    ASSET_TYPES(X)
};
#undef X

void Serialize(SerdeArchive* sa, AssetParams* options) {
#define X(enum_name, ...) Serde(sa, #enum_name, &options->enum_name##Params);
    ASSET_TYPES(X)
#undef X
}

template <typename T>
bool LoadAssetParams(AssetRegistry* assets,
                     String asset_path,
                     String param_name,
                     String extension,
                     T* out) {
    auto scratch = GetScratchArena();

    ResetStruct(out);
    String full_asset_filepath = GetFullAssetPath(assets->AssetLoadingArena, assets, asset_path);
    String options_filepath = paths::ChangeExtension(scratch, full_asset_filepath, extension);

    if (auto data = LoadFile(scratch, options_filepath); !data.empty()) {
        SerdeArchive sa =
            NewSerdeArchive(scratch, scratch, ESerdeBackend::YAML, ESerdeMode::Deserialize);
        Load(&sa, data);
        Serde(&sa, param_name.Str(), out);
    } else {
        // Otherwise we create it.
        SerdeArchive sa =
            NewSerdeArchive(scratch, scratch, ESerdeBackend::YAML, ESerdeMode::Serialize);
        Serde(&sa, param_name.Str(), out);

        String yaml_str = GetSerializedString(scratch, sa);
        bool ok = SaveFile(options_filepath, yaml_str.ToSpan());
        ASSERT(ok);
    }

    return true;
}

AssetHandle DeserializeAssetFromDisk(AssetRegistry* assets,
                                     EAssetType asset_type,
                                     String asset_path) {
    auto scratch = GetScratchArena();

#define X(enum_name, struct_name, ...)                        \
    case EAssetType::enum_name: {                             \
        Create##enum_name##Params params;                     \
        if (!LoadAssetParams(assets,                          \
                             asset_path,                      \
                             String(#enum_name),              \
                             String("." #enum_name ".yml"),   \
                             &params)) {                      \
            ASSERT(false);                                    \
            return {};                                        \
        }                                                     \
        return Create##enum_name(assets, asset_path, params); \
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

// IMGUI -------------------------------------------------------------------------------------------

namespace asset_registry_private {

template <typename T>
void BuildImGuiForAssetHolder(T* holder) {
    if (ImGui::BeginListBox("Entities",
                            ImVec2(-FLT_MIN, 10 * ImGui::GetTextLineHeightWithSpacing()))) {
        for (i32 i = 0; i < holder->Assets.Size; i++) {
            auto& asset = holder->Assets[i];
            if (!IsValid(asset)) {
                continue;
            }
            String display = Printf(GetScratchArena(),
                                    "%04d: ID: %d, ASSET_PATH: %s",
                                    i,
                                    asset.AssetID,
                                    asset.AssetPath.Str());
            ImGui::Selectable(display.Str(), false);
        }
        ImGui::EndListBox();
    }
}

}  // namespace asset_registry_private

void BuildImGuiForAssetType(AssetRegistry* assets, EAssetType asset_type) {
    using namespace asset_registry_private;

#define X(enum_name, struct_name, ...)                          \
    case EAssetType::enum_name: {                               \
        BuildImGuiForAssetHolder(&assets->struct_name##Holder); \
        return;                                                 \
    }

    switch (asset_type) {
        ASSET_TYPES(X)
        case EAssetType::Invalid: ASSERT(false); return;
        case EAssetType::COUNT: ASSERT(false); return;
    }

#undef X
}

// VALIDATIONS -------------------------------------------------------------------------------------

#define X(enum_name, struct_name, ...)                                                          \
    static_assert(Create##enum_name##Params::kCreateAssetStructRequiresGENERATE_ASSET_PARAMS,   \
                  "CreateAssetParams requires you to add the GENERATE_ASSET_PARAMS macro. See " \
                  "CreateTextureParams as an example.");
ASSET_TYPES(X)
#undef X

}  // namespace kdk

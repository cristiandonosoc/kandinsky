#include <kandinsky/asset_registry.h>

#include <kandinsky/core/file.h>
#include <kandinsky/core/string.h>
#include <kandinsky/platform.h>

namespace kdk {

// BASE ASSETS -------------------------------------------------------------------------------------

namespace asset_registry_private {

bool LoadBaseMaterials(AssetRegistry* assets) {
    // We create a fake white material.

    CreateMaterialParams white_material_params{
        .AssetOptions = {.IsBaseAsset = true},
        .Albedo = ToVec3(Color32::White),
    };
    assets->BaseAssets.WhiteMaterialHandle =
        CreateMaterial(assets, String("/Basic/Materials/White"), white_material_params);

    return true;
}

bool LoadBaseShaders(PlatformState* ps, AssetRegistry* assets) {
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
        SDL_Log("ERROR: Creating light shader from %s", path.Str());
        return false;
    }

    if (assets->BaseAssets.LineBatcherShaderHandle =
            CreateShader(&ps->Assets, String("shaders/line_batcher.glsl"), shader_params);
        !IsValid(assets->BaseAssets.LineBatcherShaderHandle)) {
        SDL_Log("ERROR: Creating line batcher shader from %s", path.Str());
        return false;
    }

    if (assets->BaseAssets.GridShaderHandle =
            CreateShader(&ps->Assets, String("shaders/grid.glsl"), shader_params);
        !IsValid(assets->BaseAssets.GridShaderHandle)) {
        SDL_Log("ERROR: Creating grid shader from %s", path.Str());
        return false;
    }

    if (assets->BaseAssets.BillboardShaderHandle =
            CreateShader(&ps->Assets, String("shaders/billboard.glsl"), shader_params);
        !IsValid(assets->BaseAssets.BillboardShaderHandle)) {
        SDL_Log("ERROR: Creating billboard shader from %s", path.Str());
        return false;
    }

    if (assets->BaseAssets.FontShaderHandle =
            CreateShader(&ps->Assets, String("shaders/font.glsl"), shader_params);
        !IsValid(assets->BaseAssets.FontShaderHandle)) {
        SDL_Log("ERROR: Creating font shader from %s", path.Str());
        return false;
    }

    return true;
}

// clang-format off
Array kCubeVertices = {
    // positions				  // normals			   // UV
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

bool LoadBaseMeshes(AssetRegistry* assets) {
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
        MeshAssetHandle cube_mesh_handle = CreateMesh(assets, "Cube"sv, params);
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
                                     "/Basic/Cube"sv,
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
                CreateModel(assets, "models/sphere/scene.gltf"sv, model_params);
            !IsValid(assets->BaseAssets.SphereModelHandle)) {
            SDL_Log("ERROR: Creating sphere mesh");
            return false;
        }
    }

    return true;
}

bool LoadInitialFonts(PlatformState* ps, AssetRegistry* assets) {
    (void)ps;

    auto scratch = GetScratchArena();

    if (assets->BaseAssets.DefaultFontHandle =
            CreateFont(assets, "fonts/roboto/Roboto-Regular.ttf"sv);
        !IsValid(assets->BaseAssets.DefaultFontHandle)) {
        SDL_Log("ERROR: Creating default font");
        return false;
    }

    return true;
}

bool LoadInitialIcons(PlatformState* ps, AssetRegistry* assets) {
    auto scratch = GetScratchArena();

    String dir_path =
        paths::PathJoin(scratch.Arena, ps->Assets.AssetBasePath, String("textures/icons"sv));

    auto entries = paths::ListDir(scratch, dir_path);
    for (const auto& entry : entries) {
        if (!entry.IsFile()) {
            continue;
        }

        // For now icons have to be PNGs.
        String extension = paths::GetExtension(scratch, entry.Path);
        if (!extension.Equals(String(".png"sv))) {
            continue;
        }

        String asset_path = RemovePrefix(scratch, entry.Path, ps->Assets.AssetBasePath);
        CreateTextureParams params{
            .AssetOptions = {.IsIcon = true},
        };
        TextureAssetHandle handle = CreateTexture(assets, asset_path, params);
        if (!IsValid(handle)) {
            SDL_Log("ERROR: Creating icon texture from %s", asset_path.Str());
            return false;
        }

        assets->BaseAssets.IconTextureHandles.Push(handle);
    }

    return true;
}

bool LoadBaseAssets(PlatformState* ps, AssetRegistry* assets) {
    if (!LoadBaseMaterials(assets)) {
        SDL_Log("ERROR: Loading base materials");
        return false;
    }

    if (!LoadBaseShaders(ps, assets)) {
        SDL_Log("ERROR: Loading base shaders");
        return false;
    }

    if (!LoadBaseMeshes(assets)) {
        SDL_Log("ERROR: Loading base meshes");
        return false;
    }

    if (!LoadInitialFonts(ps, assets)) {
        SDL_Log("ERROR: Loading base icons");
        return false;
    }

    if (!LoadInitialIcons(ps, assets)) {
        SDL_Log("ERROR: Loading base icons");
        return false;
    }

    return true;
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

#define X(ENUM_NAME, STRUCT_NAME, ...)                                \
    case EAssetType::ENUM_NAME: {                                     \
        return assets->STRUCT_NAME##Holder.FindAssetHandle(asset_id); \
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

std::pair<const Asset*, void*> FindAssetOpaque(AssetRegistry* assets, AssetHandle handle) {
#define X(ENUM_NAME, STRUCT_NAME, ...)                                            \
    case EAssetType::ENUM_NAME: {                                                 \
        if (STRUCT_NAME* found = assets->STRUCT_NAME##Holder.FindAsset(handle)) { \
            return {&found->GetAsset(), found};                                   \
        } else {                                                                  \
            return {nullptr, nullptr};                                            \
        }                                                                         \
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

#define X(ENUM_NAME, ...) Create##ENUM_NAME##Params ENUM_NAME##Params;
struct AssetParams {
    ASSET_TYPES(X)
};
#undef X

void Serialize(SerdeArchive* sa, AssetParams* options) {
#define X(ENUM_NAME, ...) Serde(sa, #ENUM_NAME, &options->ENUM_NAME##Params);
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

#define X(ENUM_NAME, STRUCT_NAME, ...)                        \
    case EAssetType::ENUM_NAME: {                             \
        Create##ENUM_NAME##Params params;                     \
        if (!LoadAssetParams(assets,                          \
                             asset_path,                      \
                             String(#ENUM_NAME),              \
                             String("." #ENUM_NAME ".yml"),   \
                             &params)) {                      \
            ASSERT(false);                                    \
            return {};                                        \
        }                                                     \
        return Create##ENUM_NAME(assets, asset_path, params); \
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
void BuildImGuiVisualizationForSelectedAsset(T* asset) {
    ImGui::Text("No visualization supported for type %s", ToString(asset->kAssetType).Str());
}

template <>
void BuildImGuiVisualizationForSelectedAsset<Texture>(Texture* texture) {
    if (IsValid(*texture)) {
        float width = Min((float)texture->Width, 256.0f);
        float height = Min((float)texture->Height, 256.0f);
        ImGui::Image((ImTextureID)texture->Handle, ImVec2(width, height));

        // Move to the same line, right after the image
        ImGui::SameLine();

        // Now you can draw widgets that will appear to the right of the image
        ImGui::BeginGroup();  // Optional: group widgets for better organization
        {
            BuildImGui(texture);
        }
        ImGui::EndGroup();
    }
}

template <typename T>
void BuildImGuiForAssetHolder(T* holder) {
    static i32 selected_index = NONE;

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
                                    asset.GetAssetID(),
                                    asset.AssetPath.Str());
            bool selected = (i == selected_index);
            if (ImGui::Selectable(display.Str(), selected)) {
                selected_index = i;
            }
        }
        ImGui::EndListBox();
    }

    if (selected_index >= 0 && selected_index < holder->Size()) {
        BuildImGuiVisualizationForSelectedAsset(&holder->UnderlyingAssets[selected_index]);
    }
}

}  // namespace asset_registry_private

void BuildImGuiForAssetType(AssetRegistry* assets, EAssetType asset_type) {
    using namespace asset_registry_private;

#define X(ENUM_NAME, STRUCT_NAME, ...)                          \
    case EAssetType::ENUM_NAME: {                               \
        BuildImGuiForAssetHolder(&assets->STRUCT_NAME##Holder); \
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

#define X(ENUM_NAME, STRUCT_NAME, ...)                                                          \
    static_assert(Create##ENUM_NAME##Params::kCreateAssetStructRequiresGENERATE_ASSET_PARAMS,   \
                  "CreateAssetParams requires you to add the GENERATE_ASSET_PARAMS macro. See " \
                  "CreateTextureParams as an example.");
ASSET_TYPES(X)
#undef X

}  // namespace kdk

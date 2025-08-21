#include <kandinsky/asset_registry.h>

#include <kandinsky/platform.h>

namespace kdk {

namespace asset_registry_private {

Asset* FindAsset(AssetRegistry* registry, i32 id) {
    ASSERT(registry);
    for (u32 i = 0; i < registry->Assets.Size; i++) {
        if (registry->Assets[i].ID == id) {
            return &registry->Assets[i];
        }
    }

    return nullptr;
}

}  // namespace asset_registry_private

AssetHandle CreateOrFindAsset(AssetRegistry* registry, EAssetType type, String asset_path) {
    using namespace asset_registry_private;
    auto* ps = platform::GetPlatformContext();
    ASSERT(ps);

    // See if the asset exists already.
    i32 id = IDFromString(asset_path.Str()) + (i32)type;
    if (Asset* asset = FindAsset(registry, id)) {
        return {.Value = asset->ID};
    }

    // Otherwise we need to create the asset.
    ASSERT(registry->Assets.Size < registry->Assets.Capacity());

    auto scratch = GetScratchArena();
    String base = paths::PathJoin(scratch.Arena, ps->BasePath, String("assets"));
    void* underlying_asset = nullptr;

    switch (type) {
        case EAssetType::Model: {
            String full_path = paths::PathJoin(scratch.Arena, base, String("models"), asset_path);
            underlying_asset = CreateModel(scratch.Arena, &ps->Models, full_path);
            if (!underlying_asset) {
                SDL_Log("ERROR: Creating or finding model asset %s\n", full_path.Str());
                return {};
            }
            break;
        }
        case EAssetType::Shader: {
            String full_path = paths::PathJoin(scratch.Arena, base, String("shaders"), asset_path);
            underlying_asset = CreateShader(&ps->Shaders, full_path);
            if (!underlying_asset) {
                SDL_Log("ERROR: Creating or finding shader asset %s\n", full_path.Str());
                return {};
            }
            break;
        }
        case EAssetType::Texture: {
            String full_path = paths::PathJoin(scratch.Arena, base, String("textures"), asset_path);
            underlying_asset = CreateTexture(&ps->Textures, asset_path.Str(), full_path.Str(), {});
            if (!underlying_asset) {
                SDL_Log("ERROR: Creating or finding texture asset %s\n", full_path.Str());
                return {};
            }
            break;
        }
        case EAssetType::Invalid: {
            ASSERT(false);
            return {};
        }
        case EAssetType::COUNT: {
            ASSERT(false);
            return {};
        }
    }

    registry->Assets.Push(Asset{
        .ID = id,
        .Type = type,
        .AssetPath = FixedString<128>(asset_path),
        .UnderlyingAsset = underlying_asset,
    });

    return {.Value = id};
}



} // namespace kdk

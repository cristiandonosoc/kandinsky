#include <kandinsky/asset_registry.h>

#include <kandinsky/platform.h>

namespace kdk {

namespace asset_registry_private {

template <typename T, u32 SIZE>
std::pair<i32, Asset*> FindAsset(AssetHolder<T, SIZE>* asset_holder, i32 id) {
    for (i32 i = 0; i < asset_holder->Assets.Size; i++) {
        auto& asset = asset_holder->Assets[i];
        if (asset.AssetID == id) {
            return {i, &asset};
        }
    }
    return {NONE, nullptr};
}

template <typename T, u32 SIZE>
AssetHandle CreateOrFindUnderlyingAsset(AssetRegistry* asset_registry,
                                        AssetHolder<T, SIZE>* asset_holder,
                                        i32 asset_id,
                                        String underlying_asset_path,
                                        void* data) {
    if (auto [index, asset] = FindAsset(asset_holder, asset_id); index != NONE) {
        return AssetHandle::Build(asset, index);
    }

    auto scratch = GetScratchArena();
    String full_asset_path = paths::PathJoin(scratch.Arena,
                                             asset_registry->AssetBasePath,
                                             String("assets"),
                                             underlying_asset_path);
    T* underlying_asset = nullptr;

    if constexpr (std::is_same_v<T, Mesh>) {
        auto* options = (CreateMeshOptions*)data;
        if (Mesh mesh = CreateMeshAsset(full_asset_path, *options); IsValid(mesh)) {
            underlying_asset = &asset_holder->UnderlyingAssets.Push(mesh);
        }
    }
    if constexpr (std::is_same_v<T, Model>) {
        // TODO(cdc): Don't use scratch arena here.
        underlying_asset = CreateModel(scratch.Arena, nullptr, full_asset_path);
    } else if constexpr (std::is_same_v<T, Shader>) {
        auto* ps = platform::GetPlatformContext();
        ASSERT(ps);
        underlying_asset = CreateShader(&ps->Shaders, full_asset_path);
    } else if constexpr (std::is_same_v<T, Texture>) {
        auto* ps = platform::GetPlatformContext();
        ASSERT(ps);
        underlying_asset =
            CreateTexture(&ps->Textures, underlying_asset_path.Str(), full_asset_path.Str(), {});
    }

    if (!underlying_asset) {
        SDL_Log("ERROR: Creating or finding underlying asset %s\n", full_asset_path.Str());
        return {};
    }

    Asset& new_asset = asset_holder->Assets.Push(Asset{
        .AssetID = asset_id,
        .Type = T::kAssetType,
        .AssetPath = underlying_asset_path,
        .UnderlyingAsset = underlying_asset,
    });
    return AssetHandle::Build(&new_asset, asset_holder->Assets.Size - 1);
}

}  // namespace asset_registry_private

AssetHandle CreateOrFindAsset(AssetRegistry* registry,
                              EAssetType type,
                              String asset_path,
                              void* data) {
    using namespace asset_registry_private;
    auto* ps = platform::GetPlatformContext();
    ASSERT(ps);

    auto scratch = GetScratchArena();

#define X(enum_name, struct_name, ...)                                        \
    case EAssetType::enum_name: {                                             \
        return CreateOrFindUnderlyingAsset(registry,                          \
                                           &registry->struct_name##Holder,    \
                                           GenerateAssetID(type, asset_path), \
                                           asset_path,                        \
                                           data);                             \
    }

    switch (type) {
        ASSET_TYPES(X)
        case EAssetType::Invalid: {
            ASSERT(false);
            return {};
        }
        case EAssetType::COUNT: {
            ASSERT(false);
            return {};
        }
    }
#undef X

    ASSERT(false);
    return {};
}

}  // namespace kdk

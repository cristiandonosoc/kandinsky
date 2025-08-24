#pragma once

#include <kandinsky/asset.h>
#include <kandinsky/graphics/model.h>
#include <kandinsky/graphics/opengl.h>

namespace kdk {

template <typename T, u32 SIZE>
struct AssetHolder {
    FixedArray<Asset, SIZE> Assets;
    FixedArray<T, SIZE> UnderlyingAssets;
};

struct AssetRegistry {
    String AssetBasePath = {};

#define X(enum_name, struct_name, max_count, ...) \
    AssetHolder<struct_name, max_count> struct_name##Holder = {};

    ASSET_TYPES(X)
#undef X

    // MeshRegistry Meshes = {};
    // ModelRegistry Models = {};
    // TextureRegistry Textures = {};
    // MaterialRegistry Materials = {};
    // ShaderRegistry Shaders = {};
};

AssetHandle CreateOrFindAsset(AssetRegistry* registry,
                              EAssetType type,
                              String asset_path,
                              void* data);
Asset* FindAsset(AssetRegistry* registry, AssetHandle handle);

template <typename T>
AssetHandleT<T> CreateOrFindAsset(AssetRegistry* registry, String asset_path) {
    return CreateOrFindAsset(registry, T::kAssetType, asset_path);
};

}  // namespace kdk

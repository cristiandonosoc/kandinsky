#pragma once

#include <kandinsky/asset.h>
#include <kandinsky/graphics/opengl.h>
#include <kandinsky/graphics/model.h>

namespace kdk {

struct AssetRegistry {
    FixedArray<Asset, 4096> Assets;

    MeshRegistry Meshes = {};
    ModelRegistry Models = {};
    TextureRegistry Textures = {};
    MaterialRegistry Materials = {};
    ShaderRegistry Shaders = {};
};

AssetHandle CreateOrFindAsset(AssetRegistry* registry, EAssetType type, String asset_path);

template <typename T>
AssetHandleT<T> CreateOrFindAsset(AssetRegistry* registry, String asset_path) {
    return CreateOrFindAsset(registry, T::kAssetType, asset_path);
};

} // namespace kdk

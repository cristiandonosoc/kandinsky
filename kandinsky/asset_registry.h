#pragma once

#include <kandinsky/asset.h>
#include <kandinsky/graphics/model.h>
#include <kandinsky/graphics/opengl.h>
#include <kandinsky/graphics/shader.h>
#include <kandinsky/graphics/texture.h>

namespace kdk {

struct Asset {
    i32 AssetID = NONE;
    EAssetType Type = EAssetType::Invalid;
    FixedString<128> AssetPath;
    void* UnderlyingAsset = nullptr;

    AssetOptions AssetOptions = {};
};

inline bool IsValid(const Asset& a) { return a.AssetID != NONE && a.Type != EAssetType::Invalid; }
i32 GenerateAssetID(EAssetType type, String asset_path);

template <typename T, u32 SIZE>
struct AssetHolder {
    FixedArray<Asset, SIZE> Assets;
    FixedArray<T, SIZE> UnderlyingAssets;

    bool IsFull() const { return Assets.Size >= SIZE; }
    AssetHandle PushAsset(i32 asset_id, String asset_path, const AssetOptions& options, T&& t);
    AssetHandle FindAssetHandle(i32 asset_id) const;
    std::pair<Asset*, T*> FindAsset(AssetHandle handle);
};

struct AssetRegistry {
    String AssetBasePath = {};
    Arena* AssetLoadingArena = nullptr;

    struct BaseAssets {
        // Grid.
        ShaderAssetHandle NormalShaderHandle = {};
        ShaderAssetHandle LightShaderHandle = {};
        ShaderAssetHandle LineBatcherShaderHandle = {};
        ShaderAssetHandle GridShaderHandle = {};
        GLuint GridVAO = GL_NONE;

        MaterialAssetHandle WhiteMaterialHandle = {};

        MeshAssetHandle CubeMeshHandle = {};

        ModelAssetHandle CubeModelHandle = {};
        ModelAssetHandle SphereModelHandle = {};
    } BaseAssets = {};

#define X(enum_name, struct_name, max_count, ...) \
    AssetHolder<struct_name, max_count> struct_name##Holder = {};

    ASSET_TYPES(X)
#undef X
};
bool Init(PlatformState* ps, AssetRegistry* assets);
void Shutdown(PlatformState* ps, AssetRegistry* assets);

String GetFullAssetPath(Arena* arena, AssetRegistry* assets, String asset_path);

AssetHandle FindAssetHandle(AssetRegistry* assets, EAssetType asset_type, String asset_path);
std::pair<Asset*, void*> FindAsset(AssetRegistry* assets, AssetHandle handle);

// Generate getter for the handles.
#define X(enum_name, struct_name, ...)                                                             \
    inline enum_name##AssetHandle Find##enum_name##Handle(AssetRegistry* assets,                   \
                                                          String asset_path) {                     \
        AssetHandle handle = FindAssetHandle(assets, EAssetType::enum_name, asset_path);           \
        return {handle};                                                                           \
    }                                                                                              \
                                                                                                   \
    inline std::pair<Asset*, struct_name*> Find##enum_name##Asset(AssetRegistry* assets,           \
                                                                  enum_name##AssetHandle handle) { \
        auto [a, t] = FindAsset(assets, handle);                                                   \
        return {a, static_cast<struct_name*>(t)};                                                  \
    }

ASSET_TYPES(X)
#undef X

AssetHandle DeserializeAssetFromDisk(AssetRegistry* assets,
                                     EAssetType asset_type,
                                     String asset_path);

// IMGUI -------------------------------------------------------------------------------------------

void BuildImGuiForAssetType(AssetRegistry* assets, EAssetType asset_type);

// TEMPLATE IMPLEMENTATIONS ------------------------------------------------------------------------

template <typename T, u32 SIZE>
AssetHandle AssetHolder<T, SIZE>::PushAsset(i32 asset_id,
                                            String asset_path,
                                            const AssetOptions& options,
                                            T&& t) {
    ASSERT(Assets.Size == UnderlyingAssets.Size);
    ASSERT(Assets.Size < SIZE);
    ASSERT(!IsValid(FindAssetHandle(asset_id)));

    i32 index = Assets.Size;

    T* underlying_asset = &UnderlyingAssets.Push(std::move(t));

    Assets.Push(Asset{
        .AssetID = asset_id,
        .Type = T::kAssetType,
        .AssetPath = asset_path,
        .UnderlyingAsset = underlying_asset,
        .AssetOptions = options,
    });
    return AssetHandle::Build(Assets[index], index);
}

template <typename T, u32 SIZE>
AssetHandle AssetHolder<T, SIZE>::FindAssetHandle(i32 asset_id) const {
    for (i32 i = 0; i < Assets.Size; i++) {
        if (Assets[i].AssetID == asset_id) {
            return AssetHandle::Build(Assets[i], i);
        }
    }
    return {};
}

template <typename T, u32 SIZE>
std::pair<Asset*, T*> AssetHolder<T, SIZE>::FindAsset(AssetHandle handle) {
    if (!IsValid(handle) || handle.GetAssetType() != T::kAssetType) {
        return {nullptr, nullptr};
    }

    i32 index = handle.GetIndex();
    ASSERT(index >= 0 || index < Assets.Size);

    Asset* asset = &Assets[index];
    ASSERT(asset->AssetID == handle.AssetID);

    T* underlying_asset = &UnderlyingAssets[index];
    return {asset, underlying_asset};
}

}  // namespace kdk

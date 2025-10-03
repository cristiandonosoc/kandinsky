#pragma once

#include <kandinsky/asset.h>
#include <kandinsky/graphics/font.h>
#include <kandinsky/graphics/model.h>
#include <kandinsky/graphics/opengl.h>
#include <kandinsky/graphics/shader.h>
#include <kandinsky/graphics/texture.h>

namespace kdk {

struct Asset {
    AssetHandle Handle = {};
    FixedString<128> AssetPath;
    void* UnderlyingAsset = nullptr;

    AssetOptions AssetOptions = {};

    i32 GetAssetID() const { return Handle.AssetID; }
    EAssetType GetAssetType() const { return Handle.GetAssetType(); }
};

inline bool IsValid(const Asset& a) { return IsValid(a.Handle); }
i32 GenerateAssetID(EAssetType type, String asset_path);

template <typename T, u32 SIZE>
struct AssetHolder {
    FixedVector<Asset, SIZE> Assets;
    FixedVector<T, SIZE> UnderlyingAssets;

    i32 Size() const { return Assets.Size; }
    bool IsFull() const { return Assets.Size >= SIZE; }
    AssetHandle PushAsset(i32 asset_id, String asset_path, const AssetOptions& options, T&& t);
    AssetHandle FindAssetHandle(i32 asset_id) const;
    T* FindAsset(AssetHandle handle);

    std::span<T> ListAssets() { return UnderlyingAssets.ToSpan(); }
};

struct AssetRegistry {
    String AssetBasePath = {};
    Arena* AssetLoadingArena = nullptr;

    struct BaseAssets {
        static constexpr i32 kMaxIcons = 128;

        // Grid.
        ShaderAssetHandle NormalShaderHandle = {};
        ShaderAssetHandle LightShaderHandle = {};
        ShaderAssetHandle LineBatcherShaderHandle = {};
        ShaderAssetHandle GridShaderHandle = {};
        ShaderAssetHandle BillboardShaderHandle = {};

        MaterialAssetHandle WhiteMaterialHandle = {};

        MeshAssetHandle CubeMeshHandle = {};

        ModelAssetHandle CubeModelHandle = {};
        ModelAssetHandle SphereModelHandle = {};

        FixedVector<TextureAssetHandle, kMaxIcons> IconTextureHandles = {};
    } BaseAssets = {};

#define X(ENUM_NAME, STRUCT_NAME, MAX_COUNT, ...) \
    AssetHolder<STRUCT_NAME, MAX_COUNT> STRUCT_NAME##Holder = {};

    ASSET_TYPES(X)
#undef X
};
bool Init(PlatformState* ps, AssetRegistry* assets);
void Shutdown(PlatformState* ps, AssetRegistry* assets);

String GetFullAssetPath(Arena* arena, AssetRegistry* assets, String asset_path);

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
    AssetHandle handle = AssetHandle::Build(T::kAssetType, asset_id, index);

    T* underlying_asset = &UnderlyingAssets.Push(std::move(t));
    Assets.Push(Asset{
        .Handle = handle,
        .AssetPath = asset_path,
        .UnderlyingAsset = underlying_asset,
        .AssetOptions = options,
    });

    Asset* asset = &Assets[index];
    underlying_asset->_AssetPtr = asset;

    return handle;
}

template <typename T, u32 SIZE>
AssetHandle AssetHolder<T, SIZE>::FindAssetHandle(i32 asset_id) const {
    for (i32 i = 0; i < Assets.Size; i++) {
        const Asset& asset = Assets[i];
        if (asset.GetAssetID() == asset_id) {
            return asset.Handle;
        }
    }
    return {};
}

template <typename T, u32 SIZE>
T* AssetHolder<T, SIZE>::FindAsset(AssetHandle handle) {
    if (!IsValid(handle)) {
        return nullptr;
    }

    if (handle.GetAssetType() != T::kAssetType) {
        return nullptr;
    }

    i32 index = handle.GetIndex();
    ASSERT(index >= 0 || index < Assets.Size);

    Asset* asset = &Assets[index];
    ASSERT(asset->Handle == handle);

    T* underlying_asset = &UnderlyingAssets[index];
    return underlying_asset;
}

}  // namespace kdk

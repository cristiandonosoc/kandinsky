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
    T* FindAsset(AssetHandle handle);
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
    } BaseAssets = {};

#define X(ENUM_NAME, STRUCT_NAME, MAX_COUNT, ...) \
    AssetHolder<STRUCT_NAME, MAX_COUNT> STRUCT_NAME##Holder = {};

    ASSET_TYPES(X)
#undef X
};
bool Init(PlatformState* ps, AssetRegistry* assets);
void Shutdown(PlatformState* ps, AssetRegistry* assets);

String GetFullAssetPath(Arena* arena, AssetRegistry* assets, String asset_path);

AssetHandle FindAssetHandle(AssetRegistry* assets, EAssetType asset_type, String asset_path);
std::pair<const Asset*, void*> FindAssetOpaque(AssetRegistry* assets, AssetHandle handle);

// Generate getter for the handles.
#define X(ENUM_NAME, STRUCT_NAME, ...)                                                   \
    inline ENUM_NAME##AssetHandle Find##ENUM_NAME##Handle(AssetRegistry* assets,         \
                                                          String asset_path) {           \
        AssetHandle handle = FindAssetHandle(assets, EAssetType::ENUM_NAME, asset_path); \
        return {handle};                                                                 \
    }                                                                                    \
                                                                                         \
    inline STRUCT_NAME* Find##ENUM_NAME##Asset(AssetRegistry* assets,                    \
                                               ENUM_NAME##AssetHandle handle) {          \
        auto [_, opaque] = FindAssetOpaque(assets, handle);                              \
        return (STRUCT_NAME*)opaque;                                                     \
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

    Asset* asset = &Assets[index];
    underlying_asset->_AssetPtr = asset;

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
    ASSERT(asset->AssetID == handle.AssetID);

    T* underlying_asset = &UnderlyingAssets[index];
    return underlying_asset;
}

}  // namespace kdk

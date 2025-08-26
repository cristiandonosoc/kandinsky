#pragma once

#include <kandinsky/asset.h>
#include <kandinsky/graphics/model.h>
#include <kandinsky/graphics/opengl.h>

namespace kdk {

template <typename T, u32 SIZE>
struct AssetHolder {
    FixedArray<Asset, SIZE> Assets;
    FixedArray<T, SIZE> UnderlyingAssets;

    bool IsFull() const { return Assets.Size >= SIZE; }

    AssetHandle PushAsset(i32 asset_id, String asset_path, T&& t) {
        ASSERT(Assets.Size == UnderlyingAssets.Size);
        ASSERT(Assets.Size < SIZE);
        ASSERT(!IsValid(FindHandle(asset_id)));

        i32 index = Assets.Size;

        T* underlying_asset = &UnderlyingAssets.Push(std::move(t));

        Assets.Push(Asset{
            .AssetID = asset_id,
            .Type = T::kAssetType,
            .AssetPath = asset_path,
            .UnderlyingAsset = underlying_asset,
        });
        return AssetHandle::Build(Assets[index], index);
    }

    AssetHandle FindHandle(i32 asset_id) const {
        for (i32 i = 0; i < Assets.Size; i++) {
            if (Assets[i].AssetID == asset_id) {
                return AssetHandle::Build(Assets[i], i);
            }
        }
        return {};
    }

    std::pair<Asset*, T*> FindByHandle(AssetHandle handle) {
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
};

struct AssetRegistry {
    String AssetBasePath = {};
    Arena* AssetLoadingArena = nullptr;

#define X(enum_name, struct_name, max_count, ...) \
    AssetHolder<struct_name, max_count> struct_name##Holder = {};

    ASSET_TYPES(X)
#undef X
};
void Init(PlatformState* ps, AssetRegistry* assets);
void Shutdown(PlatformState* ps, AssetRegistry* assets);

String GetFullAssetPath(Arena* arena, AssetRegistry* assets, String asset_path);

AssetHandle FindAsset(AssetRegistry* assets, EAssetType asset_type, String asset_path);

// Generate getter for the handles.
#define X(enum_name, ...)                                                                     \
    inline enum_name##AssetHandle Find##enum_name(AssetRegistry* assets, String asset_path) { \
        AssetHandle handle = FindAsset(assets, EAssetType::enum_name, asset_path);            \
        return {handle};                                                                      \
    }
ASSET_TYPES(X)
#undef X

std::pair<Asset*, void*> FindUnderlyingAsset(AssetRegistry* registry, AssetHandle handle);

template <typename T>
std::pair<Asset*, T*> FindUnderlyingAssetT(AssetRegistry* registry, AssetHandle handle) {
    auto [a, t] = FindUnderlyingAsset(registry, handle);
    return {a, static_cast<T*>(t)};
}

}  // namespace kdk

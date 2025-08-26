#include <kandinsky/asset_registry.h>

#include <kandinsky/platform.h>
#include "kandinsky/core/string.h"

namespace kdk {

namespace asset_registry_private {

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

void Init(PlatformState* ps, AssetRegistry* assets) {
    assets->AssetBasePath =
        paths::PathJoin(ps->Memory.StringArena.GetPtr(), ps->BasePath, String("assets"sv));
    assets->AssetLoadingArena = ps->Memory.AssetLoadingArena.GetPtr();
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
    i32 asset_id = GenerateAssetID(EAssetType::Mesh, asset_path);

#define X(enum_name, struct_name, ...)                           \
    case EAssetType::enum_name: {                                \
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
#define X(enum_name, struct_name, ...)                           \
    case EAssetType::enum_name: {                                \
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

}  // namespace kdk

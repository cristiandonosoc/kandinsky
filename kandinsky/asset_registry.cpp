#include <kandinsky/asset_registry.h>

#include <kandinsky/platform.h>

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

std::pair<Asset*, void*> FindUnderlyingAsset(AssetRegistry* registry, AssetHandle handle) {
#define X(enum_name, struct_name, ...)                             \
    case EAssetType::enum_name: {                                  \
        return registry->struct_name##Holder.FindByHandle(handle); \
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

#include <kandinsky/asset.h>

#include <kandinsky/core/serde.h>
#include <kandinsky/platform.h>

namespace kdk {

i32 GenerateAssetID(EAssetType type, String asset_path) {
    return IDFromString(asset_path.Str()) + (i32)type;
}

AssetHandle AssetHandle::Build(Asset* asset, i32 index) {
    return AssetHandle{
        .Value = ((i32)asset->Type << 24) | (index & 0xFFFFFF),
        .AssetID = asset->AssetID,
    };
}

EAssetType AssetHandle::GetAssetType() const {
    if (Value == NONE) {
        return EAssetType::Invalid;
    }
    EAssetType type = static_cast<EAssetType>((u8)(Value >> 24));
    ASSERTF(type > EAssetType::Invalid && type < EAssetType::COUNT,
            "Invalid asset type: %d",
            (u8)type);
    return type;
}

void SerializeAssetHandle(SerdeArchive* sa, EAssetType type, AssetHandle* handle) {
    (void)sa;
    (void)type;
    (void)handle;
    ASSERTF(false, "Not implemented yet");
}

}  // namespace kdk

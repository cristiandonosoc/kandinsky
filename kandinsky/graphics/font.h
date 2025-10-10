#pragma once

#include <kandinsky/asset.h>

namespace kdk {

struct Font {
    GENERATE_ASSET(Font);

    TextureAssetHandle AtlasTextureHandle = {};
};

struct CreateFontParams {
    GENERATE_ASSET_PARAMS();
};
inline void Serialize(SerdeArchive*, CreateFontParams*) {}

FontAssetHandle CreateFont(AssetRegistry* assets,
                           String asset_path,
                           const CreateFontParams& params = {});

}  // namespace kdk

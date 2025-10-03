#include <kandinsky/graphics/font.h>

namespace kdk {

FontAssetHandle CreateFont(AssetRegistry* assets,
                           String asset_path,
                           const CreateFontParams& params) {
    (void)assets;
    (void)asset_path;
    (void)params;
    UNIMPLEMENTED();
    return {};
}

}  // namespace kdk

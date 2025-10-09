#include <kandinsky/graphics/font.h>

#include <kandinsky/asset_registry.h>
#include <kandinsky/core/file.h>
#include <kandinsky/core/memory.h>

#include <stb/stb_truetype.h>

namespace kdk {

FontAssetHandle CreateFont(AssetRegistry* assets,
                           String asset_path,
                           const CreateFontParams& params) {
    if (FontAssetHandle found = FindFontHandle(assets, asset_path); IsValid(found)) {
        return found;
    }

    ASSERT(!assets->FontHolder.IsFull());

    auto scratch = GetScratchArena();
    String full_asset_path = GetFullAssetPath(scratch, assets, asset_path);

    ScopedArena scoped_arena = GetScopedArena(assets->AssetLoadingArena);
    auto data = LoadFile(scoped_arena, full_asset_path);
    if (data.empty()) {
        return {};
    }

    (void)params;
    UNIMPLEMENTED();
    return {};
}

}  // namespace kdk

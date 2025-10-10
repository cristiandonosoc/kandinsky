#include <imgui.h>
#include <kandinsky/graphics/font.h>

#include <kandinsky/asset_registry.h>
#include <kandinsky/core/file.h>
#include <kandinsky/core/memory.h>

#include <stb/stb_truetype.h>

#include <SDL3/SDL_log.h>

namespace kdk {

FontAssetHandle CreateFont(AssetRegistry* assets,
                           String asset_path,
                           const CreateFontParams& params) {
    (void)params;

    if (FontAssetHandle found = FindFontHandle(assets, asset_path); IsValid(found)) {
        return found;
    }

    ASSERT(!assets->FontHolder.IsFull());

    auto scratch = GetScratchArena();
    String full_asset_path = GetFullAssetPath(scratch, assets, asset_path);

    ScopedArena scoped_arena = GetScopedArena(assets->AssetLoadingArena);

    auto font_data = LoadFile(scoped_arena, full_asset_path);
    if (font_data.empty()) {
        SDL_Log("ERROR: Loading file %s\n", full_asset_path.Str());
        return {};
    }
    SDL_Log("Loaded %d bytes from %s\n", (i32)font_data.size_bytes(), full_asset_path.Str());

    i32 font_count = stbtt_GetNumberOfFonts(font_data.data());
    if (font_count == -1) {
        SDL_Log("ERROR: No fonts found in %s\n", full_asset_path.Str());
        return {};
    }
    SDL_Log("Found %d fonts in %s\n", font_count, full_asset_path.Str());

    // 8 bits per pixel.
    i32 width = 1024;
    i32 height = 1024;
    i32 atlas_size = width * height;
    u8* atlas_buffer = ArenaPush(scoped_arena, atlas_size);

    // There are 95 ASCII characters from ASCII 32(Space) to ASCII 126(~)
    // ASCII 32(Space) to ASCII 126(~) are the commonly used characters in text
    constexpr u32 code_point_of_first_char = 32;
    constexpr u32 chars_to_include_in_font_atlas = 95;
    // Font pixel height
    constexpr float font_size = 64.0f;

    auto packed_chars =
        ArenaPushArray<stbtt_packedchar>(scoped_arena, chars_to_include_in_font_atlas);
    auto aligned_quads =
        ArenaPushArray<stbtt_aligned_quad>(scoped_arena, chars_to_include_in_font_atlas);

    stbtt_pack_context pack_context;

    bool ok = false;

    ok = stbtt_PackBegin(&pack_context,  // stbtt_pack_context (this call will initialize it)
                         atlas_buffer,   // Font Atlas bitmap data
                         width,          // Width of the font atlas texture
                         height,         // Height of the font atlas texture
                         0,              // Stride in bytes
                         1,              // Padding between the glyphs
                         nullptr);
    if (!ok) {
        SDL_Log("ERROR: stbtt_PackBegin failed\n");
        return {};
    }

    stbtt_PackFontRange(
        &pack_context,     // stbtt_pack_context
        font_data.data(),  // Font Atlas texture data
        0,                 // Font Index
        font_size,         // Size of font in pixels. (Use STBTT_POINT_SIZE(fontSize) to use points)
        code_point_of_first_char,        // Code point of the first character
        chars_to_include_in_font_atlas,  // No. of charecters to be included in the font atlas
        packed_chars.data()              // this struct will contain the data to render a glyph
    );
    if (!ok) {
        SDL_Log("ERROR: stbtt_PackFontRange failed\n");
        return {};
    }

    stbtt_PackEnd(&pack_context);

    for (u32 i = 0; i < chars_to_include_in_font_atlas; i++) {
        float unusedx, unusedy;

        stbtt_GetPackedQuad(packed_chars.data(),  // Array of stbtt_packedchar
                            width,                // Width of the font atlas texture
                            height,               // Height of the font atlas texture
                            i,                    // Index of the glyph
                            &unusedx,
                            &unusedy,  // current position of the glyph in screen pixel coordinates,
                                       // (not required as we have a different corrdinate system)
                            &aligned_quads[i],  // stbtt_alligned_quad struct. (this struct mainly
                                                // consists of the texture coordinates)
                            0  // Allign X and Y position to a integer (doesn't matter because we
                               // are not using 'unusedX' and 'unusedY')
        );
    }

    CreateTextureParams texture_params = {
        .TextureType = ETextureType::FontAtlas,
        .DataBuffer{
                    .Width = width,
                    .Height = height,
                    .Channels = 1,
                    .Buffer = {atlas_buffer, (u32)atlas_size},
                    },
        .LoadFromDataBuffer = true,
    };

    String atlas_path = Printf(scratch, "TEXTURE_ATLAS:%s", asset_path.Str());
    TextureAssetHandle atlas_handle = CreateTexture(assets, atlas_path, texture_params);
    if (!IsValid(atlas_handle)) {
        SDL_Log("ERROR: Creating texture for font atlas %s\n", atlas_path.Str());
        return {};
    }

    i32 asset_id = GenerateAssetID(EAssetType::Texture, asset_path);
    Font font{
        .AtlasTextureHandle = atlas_handle,
    };

    SDL_Log("Created font %s with asset id %d\n", asset_path.Str(), asset_id);
    AssetHandle result =
        assets->FontHolder.PushAsset(asset_id, asset_path, params.AssetOptions, std::move(font));

    return {result};
}

}  // namespace kdk

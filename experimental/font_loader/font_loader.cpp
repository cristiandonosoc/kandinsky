#include <kandinsky/core/file.h>
#include <kandinsky/core/memory.h>

#include <stb/stb_image_write.h>
#include <stb/stb_truetype.h>

#include <SDL3/SDL_Log.h>

#include <filesystem>

using namespace kdk;

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        SDL_Log("Usage: %s <cwd> <font file>\n", argv[0]);
        return 1;
    }

    String cwd = String(argv[1]);

    std::error_code ec;
    if (std::filesystem::current_path(cwd.Str(), ec); ec) {
        SDL_Log("ERROR: %s\n", ec.message().c_str());
        return -1;
    }

    String font_path = String(argv[2]);

    SDL_Log("Font path: %s\n", font_path.Str());

    Arena arena = AllocateArena("Arena"sv, 64 * MEGABYTE);
    DEFER { FreeArena(&arena); };

    auto font_data = LoadFile(&arena, font_path);
    if (font_data.empty()) {
        SDL_Log("ERROR: Loading file %s\n", font_path.Str());
        return 1;
    }
    SDL_Log("Loaded %d bytes from %s\n", (i32)font_data.size_bytes(), font_path.Str());

    i32 font_count = stbtt_GetNumberOfFonts(font_data.data());
    if (font_count == -1) {
        SDL_Log("ERROR: No fonts found in %s\n", font_path.Str());
        return 1;
    }
    SDL_Log("Found %d fonts in %s\n", font_count, font_path.Str());

    // 8 bits per pixel.
    u32 width = 1024;
    u32 height = 1024;
    u32 atlas_size = width * height;
    auto atlas_buffer = ArenaPush(&arena, atlas_size);
    ASSERT(atlas_buffer.size_bytes() == (u64)atlas_size);

    // There are 95 ASCII characters from ASCII 32(Space) to ASCII 126(~)
    // ASCII 32(Space) to ASCII 126(~) are the commonly used characters in text
    constexpr u32 code_point_of_first_char = 32;
    constexpr u32 chars_to_include_in_font_atlas = 95;
    // Font pixel height
    constexpr float font_size = 64.0f;

    auto packed_chars = ArenaPushArray<stbtt_packedchar>(&arena, chars_to_include_in_font_atlas);
    auto aligned_quads = ArenaPushArray<stbtt_aligned_quad>(&arena, chars_to_include_in_font_atlas);

    stbtt_pack_context pack_context;

    bool ok = false;

    ok = stbtt_PackBegin(&pack_context,        // stbtt_pack_context (this call will initialize it)
                         atlas_buffer.data(),  // Font Atlas bitmap data
                         width,                // Width of the font atlas texture
                         height,               // Height of the font atlas texture
                         0,                    // Stride in bytes
                         1,                    // Padding between the glyphs
                         nullptr);
    if (!ok) {
        SDL_Log("ERROR: stbtt_PackBegin failed\n");
        return 1;
    }

    ok = stbtt_PackFontRange(
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
        return 1;
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

    int result = stbi_write_png("atlas.png", width, height, 1, atlas_buffer.data(), width);
    if (result == -1) {
        SDL_Log("ERROR: Writing PNG\n");
        return 1;
    }

    SDL_Log("Wrote atlas.png\n");
    return 0;
}

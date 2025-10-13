#include <kandinsky/graphics/font.h>

#include <kandinsky/asset_registry.h>
#include <kandinsky/core/file.h>
#include <kandinsky/core/memory.h>
#include <kandinsky/core/string.h>
#include <kandinsky/platform.h>

#include <SDL3/SDL_log.h>

namespace kdk {

namespace font_private {}  // namespace font_private

FontAssetHandle CreateFont(AssetRegistry* assets,
                           String asset_path,
                           const CreateFontParams& params) {
    using namespace font_private;
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
    i32 width = (i32)Font::kAtlasTextureSize.x;
    i32 height = (i32)Font::kAtlasTextureSize.y;
    i32 atlas_size = width * height;
    auto atlas_buffer = ArenaPush(scoped_arena, atlas_size);
    ASSERT(atlas_buffer.size_bytes() == (u64)atlas_size);

    // There are 95 ASCII characters from ASCII 32(Space) to ASCII 126(~)
    // ASCII 32(Space) to ASCII 126(~) are the commonly used characters in text

    PlatformState* ps = platform::GetPlatformContext();

    auto [packed_chars_memory, packed_chars_metadata] =
        AllocateBlock(&ps->Memory.BlockArenaManager,
                      sizeof(stbtt_packedchar) * Font::kCharsToIncludeInFontAtlas);
    std::span<stbtt_packedchar> packed_chars = {(stbtt_packedchar*)packed_chars_memory.data(),
                                                Font::kCharsToIncludeInFontAtlas};
    packed_chars_metadata->ContextMsg = Printf(scratch, "Packed Chars. Font: %s", asset_path.Str());

    auto [aligned_quads_memory, aligned_quads_metadata] =
        AllocateBlock(&ps->Memory.BlockArenaManager,
                      sizeof(stbtt_aligned_quad) * Font::kCharsToIncludeInFontAtlas);
    std::span<stbtt_aligned_quad> aligned_quads = {(stbtt_aligned_quad*)aligned_quads_memory.data(),
                                                   Font::kCharsToIncludeInFontAtlas};
    aligned_quads_metadata->ContextMsg =
        Printf(scratch, "Aligned Quads. Font: %s", asset_path.Str());

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
        return {};
    }

    stbtt_PackFontRange(
        &pack_context,     // stbtt_pack_context
        font_data.data(),  // Font Atlas texture data
        0,                 // Font Index
        Font::kFontSize,   // Size of font in pixels. (Use STBTT_POINT_SIZE(fontSize) to use points)
        Font::kCodePointOfFirstChar,       // Code point of the first character
        Font::kCharsToIncludeInFontAtlas,  // No. of charecters to be included in the font atlas
        packed_chars.data()                // this struct will contain the data to render a glyph
    );
    if (!ok) {
        SDL_Log("ERROR: stbtt_PackFontRange failed\n");
        return {};
    }

    stbtt_PackEnd(&pack_context);

    for (u32 i = 0; i < Font::kCharsToIncludeInFontAtlas; i++) {
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
                    .Buffer = atlas_buffer,
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
        .PackedChars = packed_chars,
        .AlignedQuads = aligned_quads,
    };

    SDL_Log("Created font %s with asset id %d\n", asset_path.Str(), asset_id);
    AssetHandle result =
        assets->FontHolder.PushAsset(asset_id, asset_path, params.AssetOptions, std::move(font));

    return {result};
}

// TEXT RENDERER -----------------------------------------------------------------------------------

namespace font_private {

void CalculateVertices(PlatformState* ps, TextRenderer* tr, const TextDrawCommand& tdc) {
    Font* font = FindFontAsset(&ps->Assets, tdc.FontHandle);
    ASSERT(font);

    Vec4 colorv4 = ToVec4(tdc.Color);
    float scale = tdc.Size / Font::kFontSize;

    Vec3 offset_pos = {};
    Array order = {0, 1, 2, 2, 3, 0};  // Two triangles per quad.

    for (char c : tdc.Text) {
        (void)c;

        u32 code_point = (u32)c - Font::Font::kCodePointOfFirstChar;
        if (code_point >= Font::kCharsToIncludeInFontAtlas) {
            code_point = 0;
        }

        stbtt_packedchar& packed_char = font->PackedChars[code_point];
        stbtt_aligned_quad& aligned_quad = font->AlignedQuads[code_point];

        Vec2 glyph_size = {
            (packed_char.x1 - packed_char.x0) * scale,
            (packed_char.y1 - packed_char.y0) * scale,
        };

        Vec2 top_left = {
            offset_pos.x + packed_char.xoff * scale,
            offset_pos.y + packed_char.yoff * scale,
        };

        Vec3 vertices[4] = {
            {               top_left.x,                  -top_left.y, 0}, // Top-left
            {top_left.x + glyph_size.x,                  -top_left.y, 0}, // Top-right
            {top_left.x + glyph_size.x, -(top_left.y + glyph_size.y), 0}, // Bottom-right
            {               top_left.x, -(top_left.y + glyph_size.y), 0}  // Bottom-left
        };

        // // Offset by the position.
        // for (Vec3& v : vertices) {
        //     v += tdc.Position;
        // }

        Vec2 uvs[4] = {
            {aligned_quad.s0, aligned_quad.t0}, // Top-left
            {aligned_quad.s1, aligned_quad.t0}, // Top-right
            {aligned_quad.s1, aligned_quad.t1}, // Bottom-right
            {aligned_quad.s0, aligned_quad.t1}  // Bottom-left
        };

        // Push them in.
        for (int i : order) {
            tr->FontVertices->Push(
                {Vec3(vertices[i].x, vertices[i].y, 0.0f), colorv4, uvs[i]});  // Top-left
        }

        // Offset the characters forward.
        offset_pos.x += packed_char.xadvance * scale;
    }
}

}  // namespace font_private

void InitTextRenderer(PlatformState* ps, TextRenderer* tr) {
    tr->FontVertices = ArenaPushInit<TextRenderer::FontVector>(&ps->Memory.PermanentArena);
    tr->TextDrawCommands =
        ArenaPushInit<TextRenderer::TextDrawCommandVector>(&ps->Memory.PermanentArena);
    tr->TextArena = CarveArena("TextRendererArena"sv, &ps->Memory.PermanentArena, 25 * MEGABYTE);

    static_assert(offsetof(FontVertex, Position) == 0);
    static_assert(offsetof(FontVertex, Color) == 3 * sizeof(float));
    static_assert(offsetof(FontVertex, UV) == 7 * sizeof(float));

    static_assert(sizeof(FontVertex::Position) == 3 * sizeof(float));
    static_assert(sizeof(FontVertex::Color) == 4 * sizeof(float));
    static_assert(sizeof(FontVertex::UV) == 2 * sizeof(float));
    static_assert(sizeof(FontVertex) == 9 * sizeof(float));

    GLuint vao = GL_NONE;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo = GL_NONE;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(FontVertex), 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1,
                          4,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(FontVertex),
                          (const void*)(offsetof(FontVertex, Color)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(FontVertex),
                          (const void*)(offsetof(FontVertex, UV)));
    glEnableVertexAttribArray(2);

    DEFER { glBindVertexArray(GL_NONE); };

    glBufferData(GL_ARRAY_BUFFER, TextRenderer::kVBOSize, nullptr, GL_DYNAMIC_DRAW);

    DEFER { glBindBuffer(GL_ARRAY_BUFFER, GL_NONE); };

    tr->VAO = vao;
    tr->VBO = vbo;
}

void ShutdownTextRenderer(PlatformState* ps, TextRenderer* tr) {
    (void)ps;
    ResetStruct(tr);
}

void StartFrame(TextRenderer* tr) {
    tr->FontVertices->Clear();
    tr->TextDrawCommands->Clear();
    ArenaReset(&tr->TextArena);
}

void EndFrame(TextRenderer* tr) { (void)tr; }

void Buffer(PlatformState* ps, TextRenderer* tr) {
    using namespace font_private;
    for (auto& tdc : *tr->TextDrawCommands) {
        tdc.BeginVertexIndex = (u32)tr->FontVertices->Size;
        CalculateVertices(ps, tr, tdc);
        tdc.EndVertexIndex = (u32)tr->FontVertices->Size;
    }
}

void Render(PlatformState* ps, TextRenderer* tr) {
    Shader* font_shader = FindShaderAsset(&ps->Assets, ps->Assets.BaseAssets.FontShaderHandle);
    ASSERT(font_shader);
    Use(*font_shader);

    glBindVertexArray(tr->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, tr->VBO);

    for (const auto& tdc : *tr->TextDrawCommands) {
        Font* font = FindFontAsset(&ps->Assets, tdc.FontHandle);
        ASSERT(font);

        Texture* atlas_texture = FindTextureAsset(&ps->Assets, font->AtlasTextureHandle);
        ASSERT(atlas_texture);
        BindTexture(*atlas_texture, GL_TEXTURE0);

        Mat4 mmodel;
        CalculateModelMatrix(tdc.Transform, &mmodel);
        ChangeModelMatrix(&ps->RenderState, mmodel);
        SetBaseUniforms(ps->RenderState, *font_shader);

        // Draw the range of vertices for this command.
        auto vertices = tr->FontVertices->Slice(tdc.BeginVertexIndex,
                                                tdc.EndVertexIndex - tdc.BeginVertexIndex);

        i32 command_draw_count = (i32)((vertices.size_bytes() / TextRenderer::kVBOSize) + 1);
        std::span<FontVertex> remainder_vertices = vertices;
        for (i32 i = 0; i < command_draw_count; i++) {
            std::span<FontVertex> batch_vertices = remainder_vertices;
            if (batch_vertices.size_bytes() > TextRenderer::kVBOSize) {
                ASSERT(i < command_draw_count - 1);
                batch_vertices =
                    remainder_vertices.first(TextRenderer::kVBOSize / sizeof(FontVertex));
                remainder_vertices = remainder_vertices.subspan(batch_vertices.size());
            } else {
                ASSERT(i == command_draw_count - 1);
            }
            glBufferSubData(GL_ARRAY_BUFFER, 0, batch_vertices.size_bytes(), batch_vertices.data());
            glDrawArrays(GL_TRIANGLES, 0, (i32)batch_vertices.size());
        }
    }

    // for (i32 i = 0; i < draw_count; i++) {
    //     std::span<FontVertex> batch_vertices = remainder_vertices;
    //     if (batch_vertices.size_bytes() > TextRenderer::kVBOSize) {
    //         ASSERT(i < draw_count - 1);
    //         batch_vertices = remainder_vertices.first(TextRenderer::kVBOSize /
    //         sizeof(FontVertex)); remainder_vertices =
    //         remainder_vertices.subspan(batch_vertices.size());
    //     } else {
    //         ASSERT(i == draw_count - 1);
    //     }

    //     glBufferSubData(GL_ARRAY_BUFFER, 0, batch_vertices.size_bytes(), batch_vertices.data());
    //     glDrawArrays(GL_TRIANGLES, 0, (i32)batch_vertices.size());
    // }
}

void CreateTextDrawCommand(TextRenderer* tr,
                           FontAssetHandle font_handle,
                           String text,
                           const Transform& transform,
                           Color32 color,
                           float size) {
    String interned = InternStringToArena(&tr->TextArena, text);

    tr->TextDrawCommands->Push({
        .FontHandle = font_handle,
        .Text = interned,
        .Transform = transform,
        .Color = color,
        .Size = size,
    });
}

}  // namespace kdk

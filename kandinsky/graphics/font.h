#pragma once

#include <kandinsky/asset.h>
#include <kandinsky/core/color.h>
#include <kandinsky/core/container.h>
#include <kandinsky/core/memory.h>

#include <GL/glew.h>
#include <stb/stb_truetype.h>

namespace kdk {

struct PlatformState;

struct Font {
    GENERATE_ASSET(Font);

    // Font pixel height in the atlas texture.
    static constexpr float kFontSize = 256.0f;
    static constexpr Vec2 kAtlasTextureSize{1024, 2048};
    static constexpr u32 kCodePointOfFirstChar = 32;
    static constexpr u32 kCharsToIncludeInFontAtlas = 95;

    TextureAssetHandle AtlasTextureHandle = {};

    std::span<stbtt_packedchar> PackedChars = {};
    std::span<stbtt_aligned_quad> AlignedQuads = {};
};

struct CreateFontParams {
    GENERATE_ASSET_PARAMS();
};
inline void Serialize(SerdeArchive*, CreateFontParams*) {}

FontAssetHandle CreateFont(AssetRegistry* assets,
                           String asset_path,
                           const CreateFontParams& params = {});

// TEXT RENDERER -----------------------------------------------------------------------------------

struct FontVertex {
    Vec3 Position = {};
    Vec4 Color = {};
    Vec2 UV = {};
};

struct TextDrawCommand {
    FontAssetHandle FontHandle = {};
    String Text = {};
    Transform Transform = {};
    Color32 Color = {};
    float Size = 1.0f;

    u32 BeginVertexIndex = 0;
    u32 EndVertexIndex = 0;
};

struct TextRenderer {
    static constexpr u32 kMaxVertices = 1 << 16l;  // 65536
    static constexpr u32 kVBOSize = kMaxVertices * sizeof(FontVertex);

    using FontVector = FixedVector<FontVertex, kMaxVertices>;
    using TextDrawCommandVector = FixedVector<TextDrawCommand, 1024>;

    FontVector* FontVertices = nullptr;
    TextDrawCommandVector* TextDrawCommands = nullptr;

    Arena TextArena = {};  // Arena to store the texts to render this frame.

    GLuint VAO = GL_NONE;
    GLuint VBO = GL_NONE;
};

void InitTextRenderer(PlatformState* ps, TextRenderer* tr);
void ShutdownTextRenderer(PlatformState* ps, TextRenderer* tr);

void StartFrame(TextRenderer* tr);
void EndFrame(TextRenderer* tr);
void Buffer(PlatformState* ps, TextRenderer* tr);
void Render(PlatformState* ps, TextRenderer* tr);

void CreateTextDrawCommand(TextRenderer* tr,
                           FontAssetHandle font_handle,
                           String text,
                           const Transform& transform,
                           Color32 color = Color32::White,
                           float size = 1.0f);

}  // namespace kdk

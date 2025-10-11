#pragma once

#include <kandinsky/asset.h>
#include <kandinsky/core/color.h>
#include <kandinsky/core/memory.h>

#include <stb/stb_truetype.h>
#include <GL/glew.h>

namespace kdk {

struct PlatformState;

struct Font {
    GENERATE_ASSET(Font);

    GLuint VAO = GL_NONE;
    GLuint VBO = GL_NONE;
    TextureAssetHandle AtlasTextureHandle = {};

    MemoryBlockHandle PackedCharsBlock = {};
    std::span<stbtt_packedchar> PackedChars = {};
    MemoryBlockHandle AlignedQuadsBlock = {};
    std::span<stbtt_aligned_quad> AlignedQuads = {};
};

struct CreateFontParams {
    GENERATE_ASSET_PARAMS();
};
inline void Serialize(SerdeArchive*, CreateFontParams*) {}

FontAssetHandle CreateFont(AssetRegistry* assets,
                           String asset_path,
                           const CreateFontParams& params = {});

void RenderText(PlatformState* ps,
                const Font& font,
                String text,
                const Vec3& position,
                Color32 color);

}  // namespace kdk

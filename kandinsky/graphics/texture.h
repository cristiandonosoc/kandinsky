#pragma once

#include <kandinsky/asset.h>
#include <kandinsky/core/string.h>

#include <GL/glew.h>

namespace kdk {

enum class ETextureType : u8 {
    None,
    Diffuse,
    Specular,
    Emissive,
};

struct Texture {
    GENERATE_ASSET(Texture);

    String Path = {};
    i32 ID = NONE;
    i32 Width = 0;
    i32 Height = 0;
    GLuint Handle = GL_NONE;
    ETextureType Type = ETextureType::None;
};
bool IsValid(const Texture& texture);

void Bind(const Texture& texture, GLuint texture_unit);

struct CreateTextureParams {
    GENERATE_ASSET_PARAMS();

    ETextureType Type = ETextureType::None;
    bool FlipVertically = false;
    GLint WrapS = GL_REPEAT;
    GLint WrapT = GL_REPEAT;
};
void Serialize(SerdeArchive* sa, CreateTextureParams* params);
TextureAssetHandle CreateTexture(AssetRegistry* assets,
                                 String asset_path,
                                 const CreateTextureParams& params = {});

}  // namespace kdk

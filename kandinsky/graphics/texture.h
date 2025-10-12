#pragma once

#include <kandinsky/asset.h>
#include <kandinsky/core/string.h>

#include <GL/glew.h>

namespace kdk {

enum class ETextureType : u8 {
    Invalid = 0,
    Diffuse,
    Specular,
    Emissive,
    FontAtlas,
    COUNT,
};
String ToString(ETextureType type);

struct Texture {
    GENERATE_ASSET(Texture);

    String Path = {};
    i32 ID = NONE;
    i32 Width = 0;
    i32 Height = 0;
    GLuint Handle = GL_NONE;
    GLuint Format = GL_NONE;
    ETextureType TextureType = ETextureType::Invalid;
};
bool IsValid(const Texture& texture);
void BuildImGui(Texture* texture);

void BindTexture(const Texture& texture, GLuint texture_unit);

struct CreateTextureParams {
    GENERATE_ASSET_PARAMS();

    ETextureType TextureType = ETextureType::Invalid;
    GLint WrapS = GL_REPEAT;
    GLint WrapT = GL_REPEAT;

    struct {
        i32 Width = 0;
        i32 Height = 0;
        i32 Channels = 0;
        std::span<u8> Buffer = {};
    } DataBuffer;

    bool FlipVertically = false;
    bool LoadFromDataBuffer : 1 = false;
};
void Serialize(SerdeArchive* sa, CreateTextureParams* params);
TextureAssetHandle CreateTexture(AssetRegistry* assets,
                                 String asset_path,
                                 const CreateTextureParams& params = {});

}  // namespace kdk

#include <kandinsky/graphics/texture.h>

#include <kandinsky/asset_registry.h>
#include <kandinsky/platform.h>

#include <stb/stb_image.h>

#include <SDL3/SDL_Log.h>

namespace kdk {

bool IsValid(const Texture& texture) {
    return texture.Width != 0 && texture.Height != 0 && texture.Handle != GL_NONE;
}

String ToString(ETextureType type) {
    switch (type) {
        case ETextureType::Invalid: return "<invalid>"sv;
        case ETextureType::Diffuse: return "Diffuse"sv;
        case ETextureType::Specular: return "Specular"sv;
        case ETextureType::Emissive: return "Emissive"sv;
        case ETextureType::FontAtlas: return "FontAtlas"sv;
        case ETextureType::COUNT: ASSERT(false); return "<count>"sv;
    }

    ASSERT(false);
    return "<unknown>"sv;
}

void BuildImGui(Texture* texture) {
    ImGui::Text("Path: %s", texture->GetAsset().AssetPath.Str());
    ImGui::Text("ID: %d", texture->GetAsset().Handle.AssetID);
    ImGui::Text("Dimensions: %dx%d", texture->Width, texture->Height);

    const char* format_str = "<unknown>";
    switch (texture->Format) {
        case GL_RED: format_str = "GL_RED"; break;
        case GL_RGB: format_str = "GL_RGB"; break;
        case GL_RGBA: format_str = "GL_RGBA"; break;
    }
    ImGui::Text("Format: %s", format_str);

    ImGui::Text("Type: %s", ToString(texture->TextureType).Str());
}

void BindTexture(const Texture& texture, GLuint texture_unit) {
    ASSERT(IsValid(texture));
    glActiveTexture(texture_unit);
    glBindTexture(GL_TEXTURE_2D, texture.Handle);
}

void Serialize(SerdeArchive* sa, CreateTextureParams* params) {
    Serde(sa, "Type", (u8*)&params->TextureType);
    SERDE(sa, params, FlipVertically);
    SERDE(sa, params, WrapS);
    SERDE(sa, params, WrapT);
}

TextureAssetHandle CreateTexture(AssetRegistry* assets,
                                 String asset_path,
                                 const CreateTextureParams& params) {
    if (TextureAssetHandle found = FindTextureHandle(assets, asset_path); IsValid(found)) {
        return found;
    }

    ASSERT(!assets->TextureHolder.IsFull());

    auto scratch = GetScratchArena();
    String full_asset_path = GetFullAssetPath(scratch, assets, asset_path);

    stbi_set_flip_vertically_on_load(params.FlipVertically);

    i32 width, height, channels;

    u8* data = nullptr;

    if (!params.LoadFromDataBuffer) {
        data = stbi_load(full_asset_path.Str(), &width, &height, &channels, 0);
        if (!data) {
            SDL_Log("ERROR: Texture (asset %s) not found at %s",
                    asset_path.Str(),
                    full_asset_path.Str());
            return {};
        }
    } else {
        width = params.DataBuffer.Width;
        height = params.DataBuffer.Height;
        channels = params.DataBuffer.Channels;
        data = params.DataBuffer.Buffer.data();
    }
    // Be sure to clean the loaded stbi image.
    DEFER {
        if (!params.LoadFromDataBuffer) {
            stbi_image_free(data);
        }
    };

    GLuint handle = GL_NONE;
    glGenTextures(1, &handle);
    if (handle == GL_NONE) {
        return {};
    }

    glBindTexture(GL_TEXTURE_2D, handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, params.WrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, params.WrapT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLuint internal_format = GL_NONE;
    GLuint format = GL_NONE;
    switch (channels) {
        case 1:
            internal_format = GL_R8;
            format = GL_RED;
            break;
        case 3:
            internal_format = GL_RGB;
            format = GL_RGB;
            break;
        case 4:
            internal_format = GL_RGBA;
            format = GL_RGBA;
            break;
        default:
            SDL_Log("ERROR: Unsupported number of channels: %d", channels);
            return {};
            break;
    }

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 internal_format,
                 width,
                 height,
                 0,
                 format,
                 GL_UNSIGNED_BYTE,
                 data);
    glGenerateMipmap(GL_TEXTURE_2D);

    i32 asset_id = GenerateAssetID(EAssetType::Texture, asset_path);
    Texture texture{
        .Path = platform::InternToStringArena(asset_path.Str()),
        .ID = asset_id,
        .Width = width,
        .Height = height,
        .Handle = handle,
        .Format = format,
        .TextureType = params.TextureType,
    };

    SDL_Log("Created texture %s\n", asset_path.Str());

    AssetHandle result = assets->TextureHolder.PushAsset(asset_id,
                                                         asset_path,
                                                         params.AssetOptions,
                                                         std::move(texture));
    return {result};
}

}  // namespace kdk

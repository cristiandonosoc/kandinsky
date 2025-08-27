#include <kandinsky/graphics/texture.h>

#include <kandinsky/asset_registry.h>
#include <kandinsky/platform.h>

#include <stb/stb_image.h>

#include <SDL3/SDL_Log.h>

namespace kdk {

bool IsValid(const Texture& texture) {
    return texture.Width != 0 && texture.Height != 0 && texture.Handle != GL_NONE;
}

void Bind(const Texture& texture, GLuint texture_unit) {
    ASSERT(IsValid(texture));
    glActiveTexture(texture_unit);
    glBindTexture(GL_TEXTURE_2D, texture.Handle);
}

TextureAssetHandle CreateTexture(AssetRegistry* assets,
                                 String asset_path,
                                 const LoadTextureOptions& options) {
    if (TextureAssetHandle found = FindTextureHandle(assets, asset_path); IsValid(found)) {
        return found;
    }

    ASSERT(!assets->TextureHolder.IsFull());

    auto scratch = GetScratchArena();
    String full_asset_path = GetFullAssetPath(scratch, assets, asset_path);

    stbi_set_flip_vertically_on_load(options.FlipVertically);

    i32 width, height, channels;
    u8* data = stbi_load(full_asset_path.Str(), &width, &height, &channels, 0);
    if (!data) {
        SDL_Log("ERROR: Texture (asset %s) not found at %s",
                asset_path.Str(),
                full_asset_path.Str());
        return {};
    }
    DEFER { stbi_image_free(data); };

    GLuint handle = GL_NONE;
    glGenTextures(1, &handle);
    if (handle == GL_NONE) {
        return {};
    }

    glBindTexture(GL_TEXTURE_2D, handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, options.WrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, options.WrapT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLuint format = GL_NONE;
    switch (channels) {
        case 3: format = GL_RGB; break;
        case 4: format = GL_RGBA; break;
        default:
            SDL_Log("ERROR: Unsupported number of channels: %d", channels);
            return {};
            break;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    i32 asset_id = GenerateAssetID(EAssetType::Texture, asset_path);
    Texture texture{
        .Path = platform::InternToStringArena(asset_path.Str()),
        .ID = asset_id,
        .Width = width,
        .Height = height,
        .Handle = handle,
        .Type = options.Type,
    };

    SDL_Log("Created texture %s\n", asset_path.Str());

    AssetHandle result = assets->TextureHolder.PushAsset(asset_id, asset_path, std::move(texture));
    return {result};
}

}  // namespace kdk

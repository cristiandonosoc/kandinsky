#include <kandinsky/core/file.h>

#include <kandinsky/core/memory.h>

#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_log.h>

namespace kdk {

bool SaveFile(String path, std::span<u8> data) {
    bool ok = SDL_SaveFile(path.Str(), data.data(), data.size_bytes());
    if (!ok) {
        SDL_Log("ERROR: Saving file %s: %s", path.Str(), SDL_GetError());
        return false;
    }
    return true;
}

std::span<u8> LoadFile(Arena* arena, String path) {
    u64 size = 0;
    void* data = SDL_LoadFile(path.Str(), &size);
    if (!data) {
        SDL_Log("ERROR: Loading file %s: %s", path.Str(), SDL_GetError());
        return {};
    }
    DEFER { SDL_free(data); };

    // SDL_LoadFile does not null-terminate the data, so we add an extra byte.
    auto result = ArenaCopy(arena, {(u8*)data, size + 1});
    if (result.empty()) {
        SDL_Log("ERROR: Copying file %s to arena", path.Str());
        return {};
    }
    result[size] = 0;  // Null-terminate the data.

    return result;
}

}  // namespace kdk

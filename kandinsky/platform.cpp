#include <kandinsky/platform.h>

#include <kandinsky/utils/defer.h>

#include <SDL3/SDL_log.h>

namespace kdk {

bool IsValid(const LoadedGameLibrary& lgl) {
    // clang-format off
    return lgl.SO != nullptr &&
		   lgl.GameInit != nullptr &&
           lgl.GameUpdate != nullptr &&
		   lgl.GameRender != nullptr;
    // clang-format on
}

LoadedGameLibrary LoadGameLibrary(PlatformState* ps, const char* so_path) {
    LoadedGameLibrary lgl = {};

    lgl.SO = SDL_LoadObject(so_path);
    if (!lgl.SO) {
        SDL_Log("ERROR: Could not find SO at \"%s\"", so_path);
        return {};
    }

#define LOAD_FUNCTION(lgl, function_name)                                       \
    {                                                                           \
        SDL_FunctionPointer pointer = SDL_LoadFunction(lgl.SO, #function_name); \
        if (pointer == NULL) {                                                  \
            SDL_Log("ERROR: Didn't find function " #function_name);             \
            return {};                                                          \
        }                                                                       \
        lgl.function_name = (bool (*)(PlatformState*))pointer;                  \
    }

    LOAD_FUNCTION(lgl, OnSharedObjectLoaded);
    LOAD_FUNCTION(lgl, OnSharedObjectUnloaded);
    LOAD_FUNCTION(lgl, GameInit);
    LOAD_FUNCTION(lgl, GameUpdate);
    LOAD_FUNCTION(lgl, GameRender);

#undef LOAD_FUNCTION

    if (!IsValid(lgl)) {
        SDL_Log("ERROR: LoadedGameLibrary is not valid!");
        SDL_UnloadObject(lgl.SO);
        return {};
    }

    if (!lgl.OnSharedObjectLoaded(ps)) {
        SDL_Log("ERROR: Calling DLLInit on loaded DLL");
        SDL_UnloadObject(lgl.SO);
        return {};
    }

    return lgl;
}

void UnloadGameLibrary(PlatformState* ps, LoadedGameLibrary* lgl) {
    if (!IsValid(*lgl)) {
        return;
    }

    DEFER {
        SDL_UnloadObject(lgl->SO);
        *lgl = {};
    };

    if (!lgl->OnSharedObjectUnloaded(ps)) {
        SDL_Log("ERROR: Unloading game DLL");
    }
}

}  // namespace kdk

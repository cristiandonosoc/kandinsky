#include <kandinsky/platform.h>

#include <kandinsky/core/time.h>
#include <kandinsky/imgui.h>

#include <SDL3/SDL_log.h>

namespace kdk {

namespace platform_private {

PlatformState* gPlatform = nullptr;

}  // namespace platform_private

void FillSerdeContext(PlatformState* ps, SerdeContext* sc) {
    sc->PlatformState = ps;
    sc->EntityManager = ps->EntityManager;
    sc->AssetRegistry = &ps->Assets;
}

namespace platform {

PlatformState* GetPlatformContext() {
    ASSERT(platform_private::gPlatform);
    return platform_private::gPlatform;
}

void SetPlatformContext(PlatformState* ps) { platform_private::gPlatform = ps; }

Arena* GetFrameArena() { return &GetPlatformContext()->Memory.FrameArena; }

Arena* GetPermanentArena() { return &GetPlatformContext()->Memory.PermanentArena; }

Arena* GetStringArena() { return &GetPlatformContext()->Memory.StringArena; }

String InternToStringArena(const char* string) {
    u64 len = std::strlen(string);
    // Extra byte for the null terminator.
    u8* ptr = ArenaPush(GetStringArena(), len + 1);
    std::memcpy(ptr, string, len + 1);
    return String((const char*)ptr, len);
}

}  // namespace platform

}  // namespace kdk

#include <kandinsky/platform.h>

#include <kandinsky/imgui.h>
#include <kandinsky/time.h>
#include <kandinsky/utils/defer.h>

#include <SDL3/SDL_log.h>

namespace kdk {

namespace platform_private {

PlatformState* gPlatform = nullptr;

}  // namespace platform_private

namespace platform {

PlatformState* GetPlatformContext() {
    ASSERT(platform_private::gPlatform);
    return platform_private::gPlatform;
}

void SetPlatformContext(PlatformState* ps) {
	platform_private::gPlatform = ps;
}

Arena* GetFrameArena() { return &GetPlatformContext()->Memory.FrameArena; }

Arena* GetPermanentArena() { return &GetPlatformContext()->Memory.PermanentArena; }

Arena* GetStringArena() { return &GetPlatformContext()->Memory.StringArena; }

const char* InternToStringArena(const char* string) {
    u64 len = std::strlen(string) + 1;  // Extra byte for the null terminator.
    u8* ptr = ArenaPush(GetStringArena(), len);
    std::memcpy(ptr, string, len);
    return (const char*)ptr;
}

}  // namespace platform

}  // namespace kdk

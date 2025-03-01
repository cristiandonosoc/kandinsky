#include <kandinsky/platform.h>

#include <kandinsky/imgui.h>
#include <kandinsky/time.h>
#include <kandinsky/utils/defer.h>

#include <SDL3/SDL_log.h>

namespace kdk {

PlatformState* gPlatform = nullptr;

void SetPlatformContext(PlatformState* ps) { gPlatform = ps; }

namespace platform {

Arena* GetFrameArena() {
    ASSERT(gPlatform);
    return &gPlatform->Memory.FrameArena;
}

Arena* GetPermanentArena() {
    ASSERT(gPlatform);
    return &gPlatform->Memory.PermanentArena;
}

Arena* GetStringArena() {
    ASSERT(gPlatform);
    return &gPlatform->Memory.StringArena;
}

const char* InternToStringArena(const char* string) {
    u64 len = std::strlen(string) + 1;  // Extra byte for the null terminator.
    u8* ptr = ArenaPush(GetStringArena(), len);
    std::memcpy(ptr, string, len);
    return (const char*)ptr;
}

}  // namespace platform

}  // namespace kdk

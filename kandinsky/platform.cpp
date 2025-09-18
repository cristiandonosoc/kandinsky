#include <kandinsky/platform.h>

#include <kandinsky/core/time.h>
#include <kandinsky/imgui.h>

#include <SDL3/SDL_log.h>
#include "kandinsky/scene.h"

namespace kdk {

namespace platform_private {

PlatformState* gPlatform = nullptr;

}  // namespace platform_private

void Init(TimeTracking* tt, u64 start_frame_ticks) {
    ResetStruct(tt);
    tt->StartFrameTicks = start_frame_ticks;
}

void Update(TimeTracking* tt, u64 current_frame_ticks, u64 last_frame_ticks) {
    tt->TotalSeconds = (current_frame_ticks - tt->StartFrameTicks) / 1'000'000'000.0f;

    if (last_frame_ticks != 0) [[unlikely]] {
        u64 delta_ticks = current_frame_ticks - last_frame_ticks;
        // Transform to seconds.
        tt->DeltaSeconds = (double)(delta_ticks) / 1'000'000'000.0f;
    }
}

void BuildImGui(TimeTracking* tt) {
    ImGui::Text("Start Frame Ticks: %llu", tt->StartFrameTicks);
    ImGui::Text("Delta Seconds: %.6f", tt->DeltaSeconds);
    ImGui::Text("Total Seconds: %.6f", tt->TotalSeconds);
}

void StartPlay(PlatformState* ps) {
    ASSERT(ps->RunningSceneType == ESceneType::Editor);

    // Copy over the scene.
    ps->GameplayScene = ps->EditorScene;
    ps->EntityManager = &ps->GameplayScene.EntityManager;
    ps->SelectedEntityID = {};

    Init(&ps->RuntimeTimeTracking, platform::GetCPUTicks());
    ps->CurrentTimeTracking = &ps->RuntimeTimeTracking;

    ps->RunningSceneType = ESceneType::Game;
    SDL_Log("Switched to Game mode");
}

void EndPlay(PlatformState* ps) {
    ASSERT(ps->RunningSceneType == ESceneType::Game);

    ps->EntityManager = &ps->EditorScene.EntityManager;
    ps->CurrentTimeTracking = &ps->EditorTimeTracking;
    ps->SelectedEntityID = {};

    ps->RunningSceneType = ESceneType::Editor;
    SDL_Log("Switched to Editor mode");
}

void FillSerdeContext(PlatformState* ps, SerdeContext* sc) {
    sc->PlatformState = ps;
    sc->EntityManager = ps->EntityManager;
    sc->AssetRegistry = &ps->Assets;
}

void SetTargetEntity(PlatformState* ps, const Entity& entity) {
    ps->SelectedEntityID = entity.ID;
    SetTarget(&ps->MainCamera, entity);
}

namespace platform {

u64 GetCPUTicks() { return SDL_GetTicksNS(); }

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

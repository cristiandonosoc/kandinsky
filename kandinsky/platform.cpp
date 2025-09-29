#include <kandinsky/platform.h>

#include <kandinsky/core/time.h>
#include <kandinsky/imgui.h>

#include <SDL3/SDL_log.h>
#include "kandinsky/scene.h"
#include "kandinsky/systems/system_manager.h"

namespace kdk {

namespace platform_private {

PlatformState* gPlatform = nullptr;

void Pause(TimeTracking* tt) { tt->LastPauseTicks = platform::GetCPUTicks(); }

void Resume(TimeTracking* tt) {
    u64 now_ticks = platform::GetCPUTicks();

    // We calculate how much time we were paused and we remove that from the seconds of the runtime,
    // because we were "frozen in time".
    double paused_duration = (double)(now_ticks - tt->LastPauseTicks) / 1'000'000'000.0f;
    tt->PauseOffsetSeconds += paused_duration;
}

}  // namespace platform_private

void Init(TimeTracking* tt, u64 start_frame_ticks) {
    ResetStruct(tt);
    tt->StartFrameTicks = start_frame_ticks;
}

void Update(TimeTracking* tt, u64 current_frame_ticks, u64 last_frame_ticks) {
    tt->TotalSeconds = (current_frame_ticks - tt->StartFrameTicks) / 1'000'000'000.0f;
    tt->TotalSeconds -= tt->PauseOffsetSeconds;

    if (last_frame_ticks != 0) [[unlikely]] {
        u64 delta_ticks = current_frame_ticks - last_frame_ticks;
        // Transform to seconds.
        tt->DeltaSeconds = (double)(delta_ticks) / 1'000'000'000.0f;
    }
}

void BuildImGui(TimeTracking* tt) {
    ImGui::Text("Start Frame Ticks: %llu", tt->StartFrameTicks);
    ImGui::Text("Last Pause Ticks: %llu", tt->LastPauseTicks);
    ImGui::Text("Pause Offset Seconds: %.6f", tt->PauseOffsetSeconds);
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
    StartSystems(&ps->Systems);
}

void PausePlay(PlatformState* ps) {
    ASSERT(ps->RunningSceneType == ESceneType::Game);

    ps->RunningSceneType = ESceneType::GamePaused;
    platform_private::Pause(&ps->RuntimeTimeTracking);
    SDL_Log("Paused Game mode");
}

void ResumePlay(PlatformState* ps) {
    ASSERT(ps->RunningSceneType == ESceneType::GamePaused);

    ps->RunningSceneType = ESceneType::Game;
    platform_private::Resume(&ps->RuntimeTimeTracking);
    SDL_Log("Resumed paused Game mode");
}

void EndPlay(PlatformState* ps) {
    ASSERT(ps->RunningSceneType == ESceneType::Game ||
           ps->RunningSceneType == ESceneType::GamePaused);

    StopSystems(&ps->Systems);

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

void SetTargetEntity(PlatformState* ps,
                     const Entity& entity,
                     const SetTargetEntityOptions& options) {
    ps->SelectedEntityID = entity.ID;
    if (options.FocusCamera) {
        SetTarget(&ps->MainCamera, entity);
    }
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

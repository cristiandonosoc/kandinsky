#include <imgui.h>
#include <kandinsky/platform.h>

#include <kandinsky/core/time.h>
#include <kandinsky/imgui.h>
#include <kandinsky/imgui_widgets.h>
#include <kandinsky/scene.h>
#include <kandinsky/systems/system_manager.h>

#include <SDL3/SDL_log.h>

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

String ToString(EEditorMode mode) {
    switch (mode) {
        case EEditorMode::Invalid: return "<invalid>"sv;
        case EEditorMode::Selection: return "Selection"sv;
        case EEditorMode::Terrain: return "Terrain"sv;
        case EEditorMode::COUNT: ASSERT(false); return "<count>"sv;
    }
    ASSERT(false);
    return "<unknown>"sv;
}

EEditorMode CycleEditorMode(EEditorMode mode) {
    EEditorMode next = EEditorMode(((u8)mode + 1) % (u8)EEditorMode::COUNT);
    if (next == EEditorMode::Invalid) {
        next = EEditorMode::Selection;
    }
    return next;
}

String ToString(ERunningMode mode) {
    switch (mode) {
        case ERunningMode::Invalid: return "<invalid>"sv;
        case ERunningMode::Editor: return "Editor"sv;
        case ERunningMode::GameRunning: return "GameRunning"sv;
        case ERunningMode::GamePaused: return "GamePaused"sv;
        case ERunningMode::GameEndRequested: return "GameEndRequested"sv;
        case ERunningMode::COUNT: ASSERT(false); return "<count>"sv;
    }
    ASSERT(false);
    return "<unknown>"sv;
}

bool IsGameRunningMode(ERunningMode mode) {
    return mode == ERunningMode::GameRunning || mode == ERunningMode::GamePaused ||
           mode == ERunningMode::GameEndRequested;
}

String ToString(EGizmoOperation operation) {
    switch (operation) {
        case EGizmoOperation::Invalid: return "<invalid>"sv;
        case EGizmoOperation::Translate: return "Translate"sv;
        case EGizmoOperation::Rotate: return "Rotate"sv;
        case EGizmoOperation::Scale: return "Scale"sv;
        case EGizmoOperation::COUNT: ASSERT(false); return "<count>"sv;
    }

    ASSERT(false);
    return "<unknown>"sv;
}

ImGuizmo::OPERATION ToImGuizmoOperation(EGizmoOperation operation) {
    switch (operation) {
        case EGizmoOperation::Invalid: ASSERT(false); return ImGuizmo::TRANSLATE;
        case EGizmoOperation::Translate: return ImGuizmo::TRANSLATE;
        case EGizmoOperation::Rotate: return ImGuizmo::ROTATE;
        case EGizmoOperation::Scale: return ImGuizmo::SCALE;
        case EGizmoOperation::COUNT: ASSERT(false); return ImGuizmo::TRANSLATE;
    }

    ASSERT(false);
    return ImGuizmo::TRANSLATE;
}

EGizmoOperation CycleGizmoOperation(EGizmoOperation operation) {
    EGizmoOperation next = EGizmoOperation(((u8)operation + 1) % (u8)EGizmoOperation::COUNT);
    if (next == EGizmoOperation::Invalid) {
        next = EGizmoOperation::Translate;
    }
    return next;
}

String ToString(EGizmoMode space) {
    switch (space) {
        case EGizmoMode::World: return "World"sv;
        case EGizmoMode::Local: return "Local"sv;
    }
    ASSERT(false);
    return "<unknown>"sv;
}

ImGuizmo::MODE ToImGuizmoMode(EGizmoMode space) {
    switch (space) {
        case EGizmoMode::World: return ImGuizmo::WORLD;
        case EGizmoMode::Local: return ImGuizmo::LOCAL;
    }
    ASSERT(false);
    return ImGuizmo::WORLD;
}

EGizmoMode CycleGizmoMode(EGizmoMode mode) {
    return (mode == EGizmoMode::World) ? EGizmoMode::Local : EGizmoMode::World;
}

void StartPlay(PlatformState* ps) {
    ASSERT(ps->EditorState.RunningMode == ERunningMode::Editor);

    if (!ValidateScene(&ps->EditorScene)) {
        SDL_Log("Cannot enter Play mode with invalid scene");
        return;
    }

    // Copy over the scene.
    CloneScene(ps->EditorScene, &ps->GameplayScene);
    StartScene(&ps->GameplayScene);
    ps->CurrentScene = &ps->GameplayScene;
    ps->EntityManager = &ps->GameplayScene.EntityManager;
    ps->SelectedEntityID = {};

    Init(&ps->RuntimeTimeTracking, platform::GetCPUTicks());
    ps->CurrentTimeTracking = &ps->RuntimeTimeTracking;

    ps->EditorState.RunningMode = ERunningMode::GameRunning;
    SDL_Log("Switched to Game mode");
    StartSystems(&ps->Systems);
}

void PausePlay(PlatformState* ps) {
    ASSERT(ps->EditorState.RunningMode == ERunningMode::GameRunning);

    ps->EditorState.RunningMode = ERunningMode::GamePaused;
    platform_private::Pause(&ps->RuntimeTimeTracking);
    SDL_Log("Paused Game mode");
}

void ResumePlay(PlatformState* ps) {
    ASSERT(ps->EditorState.RunningMode == ERunningMode::GamePaused);

    ps->EditorState.RunningMode = ERunningMode::GameRunning;
    platform_private::Resume(&ps->RuntimeTimeTracking);
    SDL_Log("Resumed paused Game mode");
}

void EndPlay(PlatformState* ps) {
    ASSERT(IsGameRunningMode(ps->EditorState.RunningMode));

    StopSystems(&ps->Systems);

    ps->CurrentScene = &ps->EditorScene;
    ps->EntityManager = &ps->EditorScene.EntityManager;
    ps->CurrentTimeTracking = &ps->EditorTimeTracking;
    ps->SelectedEntityID = {};

    ps->EditorState.RunningMode = ERunningMode::Editor;
    SDL_Log("Switched to Editor mode");
}

void BuildImGui(struct PlatformState::Memory* memory) {
    if (ImGui::TreeNodeEx(memory->PermanentArena.Name.Str(), ImGuiTreeNodeFlags_Framed)) {
        BuildImGui(&memory->PermanentArena);
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx(memory->StringArena.Name.Str(), ImGuiTreeNodeFlags_Framed)) {
        BuildImGui(&memory->StringArena);
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx(memory->AssetLoadingArena.Name.Str(), ImGuiTreeNodeFlags_Framed)) {
        BuildImGui(&memory->AssetLoadingArena);
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx(memory->FrameArena.Name.Str(), ImGuiTreeNodeFlags_Framed)) {
        BuildImGui(&memory->FrameArena);
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("BlockArenaManager", ImGuiTreeNodeFlags_Framed)) {
        BuildImGui(&memory->BlockArenaManager);
        ImGui::TreePop();
    }
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

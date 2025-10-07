#pragma once

#include <kandinsky/asset_registry.h>
#include <kandinsky/core/defines.h>
#include <kandinsky/core/memory.h>
#include <kandinsky/gameplay/gamemode.h>
#include <kandinsky/gameplay/terrain.h>
#include <kandinsky/graphics/model.h>
#include <kandinsky/graphics/opengl.h>
#include <kandinsky/graphics/render_state.h>
#include <kandinsky/imgui.h>
#include <kandinsky/input.h>
#include <kandinsky/scene.h>
#include <kandinsky/systems/system_manager.h>
#include <kandinsky/window.h>

namespace kdk {

// PlatformState -----------------------------------------------------------------------------------

struct LoadedGameLibrary {
    SDL_SharedObject* SO = nullptr;
    SDL_Time SOModifiedTime = {};

    bool (*__KDKEntryPoint_OnSharedObjectLoaded)(PlatformState* ps) = nullptr;
    bool (*__KDKEntryPoint_OnSharedObjectUnloaded)(PlatformState* ps) = nullptr;
    bool (*__KDKEntryPoint_GameInit)(PlatformState* ps) = nullptr;
    bool (*__KDKEntryPoint_GameUpdate)(PlatformState* ps) = nullptr;
    bool (*__KDKEntryPoint_GameRender)(PlatformState* ps) = nullptr;
};
bool IsValid(const LoadedGameLibrary& game_lib);

struct TimeTracking {
    // The cpu ticks at the moment this time tracking was started.
    u64 StartFrameTicks = 0;
    // The cpu ticks at the moment the time tracking was paused.
    u64 LastPauseTicks = 0;
    // How much time we have been paused, which have to be offseted.
    double PauseOffsetSeconds = 0;

    double DeltaSeconds = 0;
    double TotalSeconds = 0;
};
void Init(TimeTracking* tt, u64 start_frame_ticks);
void Update(TimeTracking* tt, u64 current_frame_ticks, u64 last_frame_ticks);
void BuildImGui(TimeTracking* tt);

enum class EEditorMode : u8 {
    Invalid = 0,
    Selection,
    Terrain,
    COUNT,
};
String ToString(EEditorMode mode);
EEditorMode CycleEditorMode(EEditorMode mode);

enum class ERunningMode : u8 {
    Invalid = 0,
    Editor,
    GameRunning,
    GamePaused,
    GameEndRequested,
    COUNT,
};
String ToString(ERunningMode mode);
bool IsGameRunningMode(ERunningMode mode);

enum class EGizmoOperation : u8 {
    Invalid = 0,
    Translate,
    Rotate,
    Scale,
    COUNT,
};
String ToString(EGizmoOperation operation);
ImGuizmo::OPERATION ToImGuizmoOperation(EGizmoOperation operation);
// Gives you the next gizmo operation in cycle order.
EGizmoOperation CycleGizmoOperation(EGizmoOperation operation);

enum class EGizmoMode : u8 {
    World,
    Local,
};
String ToString(EGizmoMode space);
ImGuizmo::MODE ToImGuizmoMode(EGizmoMode space);
EGizmoMode CycleGizmoMode(EGizmoMode mode);

struct ImGuiState {
    ImGuiContext* Context = nullptr;
    ImGuiMemAllocFunc AllocFunc = nullptr;
    ImGuiMemFreeFunc FreeFunc = nullptr;

    EGizmoOperation GizmoOperation = EGizmoOperation::Translate;
    EGizmoMode GizmoMode = EGizmoMode::World;

    bool ShowEntityListWindow = false;
    bool ShowEntityDebuggerWindow = false;
    bool ShowTerrainWindow = false;
    bool ShowCameraWindow = false;
    bool ShowCameraDebugDraw = false;
    bool ShowInputWindow = false;
    bool ShowTimingsWindow = false;
    bool ShowScheduleWindow = false;

    bool EntityDraggingPressed = false;
    bool EntityDraggingDown = false;
    bool EntityDraggingReleased = false;
    Array<bool, (u8)EAssetType::COUNT> ShowAssetWindow = {};

    // Model matrix before manipulation (for snapping support)
    Transform PreDragTransform = {};
};

struct EditorState {
    EEditorMode EditorMode = EEditorMode::Selection;
    ERunningMode RunningMode = ERunningMode::Editor;

    struct {
        i32 BrushSize = 3;
		ETerrainTileType TileType = ETerrainTileType::Grass;
    } TerrainModeState;
};

struct PlatformState {
    String BasePath;

    Window Window = {};
    InputState InputState = {};

    bool ShouldExit = false;

    u64 LastFrameTicks = 0;
    // double Seconds = 0;
    // double FrameDelta = 0;

    TimeTracking EditorTimeTracking = {};
    TimeTracking RuntimeTimeTracking = {};
    TimeTracking* CurrentTimeTracking = {};

    GameMode GameMode = {};
    SystemManager Systems = {};

    Vec3 ClearColor = Vec3(0.2f);

    bool MainCameraMode = true;
    Camera MainCamera = {};
    Camera DebugCamera = {};
    Camera* CurrentCamera = nullptr;

    ImGuiState ImGuiState = {};

    // Debug FBO (for debug camera mode).
    GLuint DebugFBO = NULL;
    GLuint DebugFBOTexture = NULL;
    GLuint DebugFBODepthStencil = NULL;

    struct Memory {
        Arena PermanentArena = {};
        Arena FrameArena = {};
        // Note that all strings allocated into this arena are *permanent*.
        Arena StringArena = {};
        Arena AssetLoadingArena = {};
    } Memory;

    struct GameLibrary {
        // At most we try for this amount of time to load a new DLL when found.
        // If not, we trap, since something is probably wrong with the reloading.
        static constexpr double kMaxLoadTimeSeconds = 5;

        // How many seconds to wait between (successful) attemps to load DLLs.
        static constexpr double kLoadThresholdSeconds = 10;

        String Path = {};
        LoadedGameLibrary LoadedLibrary = {};
        SDL_Time LastLibraryTimestamp = 0;

        FixedString<512> TargetLoadPath = {};

        double LastLoadTime = 0;
        double LoadAttempStart = 0;
    } GameLibrary;

    struct ShaderLoading {
        // How many seconds to wait between (successful) attemps to load DLLs.
        static constexpr double kLoadThresholdSeconds = 10;

        double LastLoadTime = 0;
        SDL_Time LastMarkerTimestamp = 0;
    } ShaderLoading;

    LineBatcherRegistry LineBatchers = {};
    LineBatcher* DebugLineBatcher = nullptr;

    EditorState EditorState = {};
    Scene EditorScene = {
        .SceneType = ESceneType::Editor,
    };
    Scene GameplayScene = {
        .SceneType = ESceneType::Game,
    };
    Terrain Terrain = {};

    EntityManager* EntityManager = nullptr;

    EntityID SelectedEntityID = {};
    EntityID HoverEntityID = {};
    EntityPicker EntityPicker = {};

    AssetRegistry Assets = {};

    // The current options of the scene being rendered.
    struct {
        GLuint GlobalVAO = GL_NONE;
    } Rendering;

    RenderState RenderState = {};

    void* GameState = nullptr;
};

void StartPlay(PlatformState* ps);
void PausePlay(PlatformState* ps);
void ResumePlay(PlatformState* ps);
void EndPlay(PlatformState* ps);

struct SerdeContext {
    PlatformState* PlatformState = nullptr;
    EntityManager* EntityManager = nullptr;
    AssetRegistry* AssetRegistry = nullptr;
};
void FillSerdeContext(PlatformState* ps, SerdeContext* sc);

struct SetTargetEntityOptions {
    bool FocusCamera = true;
};
void SetTargetEntity(PlatformState* ps,
                     const Entity& entity,
                     const SetTargetEntityOptions& options = {});

namespace platform {

u64 GetCPUTicks();

PlatformState* GetPlatformContext();

// Use for newly loaded DLLs.
void SetPlatformContext(PlatformState* ps);

Arena* GetFrameArena();
Arena* GetPermanentArena();

Arena* GetStringArena();
String InternToStringArena(const char* string);

}  // namespace platform

#ifdef __cplusplus
extern "C" {
#endif

__declspec(dllexport) bool __KDKEntryPoint_OnSharedObjectLoaded(PlatformState* ps);
__declspec(dllexport) bool __KDKEntryPoint_OnSharedObjectUnloaded(PlatformState* ps);
__declspec(dllexport) bool __KDKEntryPoint_GameInit(PlatformState* ps);
__declspec(dllexport) bool __KDKEntryPoint_GameUpdate(PlatformState* ps);
__declspec(dllexport) bool __KDKEntryPoint_GameRender(PlatformState* ps);

#ifdef __cplusplus
}
#endif

}  // namespace kdk

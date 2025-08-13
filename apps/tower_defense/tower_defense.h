#pragma once

#include <kandinsky/camera.h>
#include <kandinsky/graphics/light.h>
#include <kandinsky/graphics/opengl.h>
#include <kandinsky/graphics/render_state.h>

namespace kdk {

struct PlatformState;
struct SerdeArchive;

static constexpr u32 kTileChunkSide = 25;
static constexpr u32 kTileChunkTotalSize = kTileChunkSide * kTileChunkSide;

enum class ETileType : u8 {
    None = 0,
    Grass,
    Road,
    COUNT,
};

struct TileChunk {
    std::array<ETileType, kTileChunkTotalSize> Tiles = {};
	std::array<Entity, kTileChunkTotalSize> Entities = {};
};

void Serialize(SerdeArchive* sa, TileChunk& tc);

inline ETileType GetTile(const TileChunk& tc, u32 x, u32 z) {
    ASSERT(x < kTileChunkSide);
    ASSERT(z < kTileChunkSide);
    return tc.Tiles[z * kTileChunkSide + x];
}

inline void SetTile(TileChunk* tc, u32 x, u32 z, ETileType tile_type) {
    ASSERT(x < kTileChunkSide);
    ASSERT(z < kTileChunkSide);
    tc->Tiles[z * kTileChunkSide + x] = tile_type;
}

// X-Macro for entity types.
#define EDITOR_MODE_TYPES(X) \
    X(Terrain)               \
    X(Tower)                 \
    X(Spawner)               \
    X(Base)

// clang-format off
#define X(name) name,


enum class EEditorMode : u8 {
    Invalid = 0,
    EDITOR_MODE_TYPES(X)
	COUNT,
};

#undef X
// clang-format on

struct ValidationError {
    Entity entity = {};
    String Message = {};
};

struct TowerDefense {
    static TowerDefense* Get();  // Defined in app.cpp.

    EntityManager EntityManager = {};

    Camera MainCamera = {
        .Position = {1.0f, 1.0f, 1.0f},
    };
    struct Camera DebugCamera = {};
    bool MainCameraMode = true;
    bool UpdateMainCameraOnDebugMode = false;

    GLuint CameraFBO = NULL;
    GLuint CameraFBOTexture = NULL;
    GLuint CameraFBODepthStencil = NULL;

    EntityPicker EntityPicker = {};
    Entity HoverEntityID = {};
	Entity SelectedEntityID = {};

	DynArray<ValidationError> ValidationErrors = {};

    TileChunk TileChunk = {};

    std::array<Material, (u32)ETileType::COUNT> Materials = {};

    ETileType SelectedTileType = ETileType::Grass;

    EEditorMode EditorMode = EEditorMode::Terrain;
};

inline u32 GridCoordID(u32 x, u32 z) { return z * kTileChunkSide + x; }
inline u32 GridCoordID(const UVec2& grid_coord) { return GridCoordID(grid_coord.x, grid_coord.y); }

void Serialize(SerdeArchive* sa, TowerDefense& td);
void EncodeDecode(const TileChunk& tc);

void Validate(Arena* arena, const TowerDefense& td, DynArray<ValidationError>* out);

}  // namespace kdk

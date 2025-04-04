#pragma once

#include <kandinsky/camera.h>
#include <kandinsky/game/entity_manager.h>
#include <kandinsky/graphics/light.h>
#include <kandinsky/graphics/opengl.h>

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

enum class EEditorMode : u8 {
	Invalid = 0,
	Terrain,
	PlaceTower,
	COUNT,
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

    // clang-format off
    DirectionalLight DirectionalLight = {
		.Direction = {-1.3f, -2.f, -1},
		.Color = {
			.Ambient = Vec3(0.5f),
			.Diffuse = Vec3(0.5f),
		},
    };
    // clang-format on

    TileChunk TileChunk = {};

    std::array<Material, (u32)ETileType::COUNT> Materials = {};

    ETileType SelectedTileType = ETileType::Grass;

	EEditorMode EditorMode = EEditorMode::Terrain;
};

void Serialize(SerdeArchive* sa, TowerDefense& td);

void EncodeDecode(const TileChunk& tc);

}  // namespace kdk

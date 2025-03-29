#pragma once

#include <kandinsky/camera.h>
#include <kandinsky/graphics/light.h>
#include <kandinsky/graphics/opengl.h>

namespace kdk {

struct PlatformState;
struct SerdeArchive;

static constexpr u32 kTileChunkSize = 25;

enum class ETileType : u8 {
    None = 0,
    Grass,
    Road,
    COUNT,
};

struct TileChunk {
    std::array<ETileType, kTileChunkSize * kTileChunkSize> Tiles = {};
};

inline ETileType GetTile(const TileChunk& tc, u32 x, u32 z) {
    ASSERT(x < kTileChunkSize);
    ASSERT(z < kTileChunkSize);
    return tc.Tiles[z * kTileChunkSize + x];
}

inline void SetTile(TileChunk* tc, u32 x, u32 z, ETileType tile_type) {
    ASSERT(x < kTileChunkSize);
    ASSERT(z < kTileChunkSize);
    tc->Tiles[z * kTileChunkSize + x] = tile_type;
}

struct TowerDefense {
    static TowerDefense* Get();	// Defined in app.cpp.

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
    Light DirectionalLight = {
        .LightType = ELightType::Directional,
		.DirectionalLight = {
			.Direction = {-1.3f, -2.f, -1},
			.Color = {
				.Ambient = Vec3(0.5f),
				.Diffuse = Vec3(0.5f),
			},
		},
    };
    // clang-format on

    TileChunk TileChunk = {};

    std::array<Material, (u32)ETileType::COUNT> Materials = {};

    ETileType SelectedTileType = ETileType::Grass;
};

void Serialize(SerdeArchive* sa, TowerDefense& td);

}  // namespace kdk

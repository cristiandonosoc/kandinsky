#pragma once

#include <kandinsky/graphics/light.h>

#include <kandinsky/math.h>
#include <kandinsky/graphics/opengl.h>

#include <bitset>

namespace kdk {

enum class EEntityType : u8 {
	Invalid = 0,
	Box,
	DirectionalLight,
	PointLight,
	Spotlight,
};

struct Entity {
	EEntityType Type = EEntityType::Invalid;
	void* Ptr = nullptr;
};

struct GameState {
	static constexpr u32 kMaxEntityCount = 128;
    Vec3 ClearColor = Vec3(0.2f);

    Camera FreeCamera = {};

	u32 EntityCount = 0;
	std::array<Entity, kMaxEntityCount> Entities = {};
	std::array<Transform, kMaxEntityCount> Transforms = {};
	std::bitset<kMaxEntityCount> DirtyTransforms = {};

	u32 HoverEntityID = (u32)NONE;
	u32 SelectedEntityID = (u32)NONE;

	// Lights.
    DirectionalLight DirectionalLight = {};
    PointLight PointLights[kNumPointLights] = {};
    Spotlight Spotlight = {};

	struct {
		float Shininess = 32.0f;
	} Material;

	std::array<Model*, 64> MiniDungeonModels = {};
	u32 MiniDungeonModelCount = 0;

	GLuint SSBO = NULL;

};

}  // namespace kdk

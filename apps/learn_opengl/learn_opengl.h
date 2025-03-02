#pragma once

#include <kandinsky/graphics/light.h>

#include <kandinsky/math.h>
#include <kandinsky/graphics/opengl.h>

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
    Vec3 ClearColor = Vec3(0.2f);

    Camera FreeCamera = {};

	Entity Entities[32] = {};
	u32 EntityCount = 0;
	u32 HoverEntityID = (u32)NONE;
	u32 SelectedEntityID = (u32)NONE;

	// Lights.
    DirectionalLight DirectionalLight = {};
    PointLight PointLights[kNumPointLights] = {};
    Spotlight Spotlight = {};

	struct {
		float Shininess = 32.0f;
	} Material;

	GLuint SSBO = NULL;

};

}  // namespace kdk

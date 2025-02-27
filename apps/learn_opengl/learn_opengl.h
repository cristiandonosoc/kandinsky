#pragma once

#include <learn_opengl/light.h>

#include <kandinsky/math.h>
#include <kandinsky/opengl.h>

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

	// Lights.
    DirectionalLight DirectionalLight = {};
	i32 SelectedLight = NONE;
    PointLight PointLights[kNumPointLights] = {};
    Spotlight Spotlight = {};

	struct {
		float Shininess = 32.0f;
	} Material;

	GLuint SSBO = NULL;
	u32 ObjectID = (u32)NONE;

};

}  // namespace kdk

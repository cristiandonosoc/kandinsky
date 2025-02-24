#pragma once

#include <learn_opengl/light.h>

#include <kandinsky/math.h>
#include <kandinsky/opengl.h>

namespace kdk {

struct GameState {
    Vec3 ClearColor = Vec3(0.4f, 0.4f, 0.4f);

    Camera FreeCamera = {};
    DirectionalLight DirectionalLight = {};

	i32 SelectedPointLight = NONE;
    PointLight PointLights[kNumPointLights] = {};
    Spotlight Spotlight = {};

	struct {
		float Shininess = 32.0f;

	} Material;
};

}  // namespace kdk

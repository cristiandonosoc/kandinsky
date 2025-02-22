#pragma once

#include <kandinsky/math.h>
#include <kandinsky/opengl.h>
#include <learn_opengl/light.h>

namespace kdk {

struct GameState {
	Vec3 ClearColor = Vec3(0.4f, 0.4f, 0.4f);


    Camera FreeCamera = {
        .Position = Vec3(-4.0f, 1.0f, 1.0f),
    };

    Light Light = {
        .Position = Vec3(1.2f, 1.0f, 2.0f),
		.MinRadius = 0.2f,
		.MaxRadius = 3.0f,

    };
};

}  // namespace kdk

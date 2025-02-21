#pragma once

#include <learn_opengl/light.h>
#include <kandinsky/opengl.h>

namespace kdk {

struct GameState {
    Camera FreeCamera = {
        .Position = glm::vec3(-4.0f, 1.0f, 1.0f),
    };

    Light Light = {
        .Position = glm::vec3(1.2f, 1.0f, 2.0f),
    };
};

}  // namespace kdk

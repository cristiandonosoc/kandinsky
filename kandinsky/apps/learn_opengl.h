#pragma once

#include <kandinsky/opengl.h>

namespace kdk {

enum class ELightType : u8 {
    Point,
    Directional,
    Spotlight,
    COUNT,
};
const char* ToString(ELightType v);

struct Light {
    glm::vec3 Position = {};
    ELightType Type = ELightType::Point;
};

struct GameState {
    Camera FreeCamera = {
        .Position = glm::vec3(-4.0f, 1.0f, 1.0f),
    };

    Light Light = {
        .Position = glm::vec3(1.2f, 1.0f, 2.0f),
    };
};

}  // namespace kdk

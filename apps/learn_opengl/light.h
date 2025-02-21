#pragma once

#include <kandinsky/defines.h>

#include <glm/glm.hpp>

namespace kdk {

struct Mesh;
struct PlatformState;
struct Shader;

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

struct RenderState_Light {
	Light* Light = nullptr;
	Shader* Shader = nullptr;
	Mesh* Mesh = nullptr;
	glm::mat4* ViewProj = nullptr;
};

void RenderLight(PlatformState* ps, RenderState_Light* rs);

}  // namespace kdk

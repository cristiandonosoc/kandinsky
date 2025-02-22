#pragma once

#include <kandinsky/defines.h>
#include <kandinsky/math.h>

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
    Vec3 Position = {};
	Vec3 Direction = {};
    ELightType Type = ELightType::Point;

    float MinRadius = 0.1f;
    float MaxRadius = 5.0f;

    struct AttenuationData {
        float Constant = 0.3f;
        float Linear = 0.09f;
        float Quadratic = 0.032f;
    } Attenuation = {};
};

void SetAttenuation(PlatformState* ps, const Shader& shader, const Light& light);

struct RenderState_Light {
    Light* Light = nullptr;
    Shader* Shader = nullptr;
    Mesh* Mesh = nullptr;
    glm::mat4* ViewProj = nullptr;
};

void RenderLight(PlatformState* ps, RenderState_Light* rs);

}  // namespace kdk

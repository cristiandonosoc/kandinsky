#pragma once

#include <kandinsky/defines.h>
#include <kandinsky/entity.h>
#include <kandinsky/math.h>

namespace kdk {

struct Mesh;
struct PlatformState;
struct Shader;
struct RenderState;

enum class ELightType : u8 {
    Invalid,
    Point,
    Directional,
    Spotlight,
    COUNT,
};
const char* ToString(ELightType v);

struct LightColor {
    Vec3 Ambient = Vec3(1.0f);
    Vec3 Diffuse = Vec3(1.0f);
    Vec3 Specular = Vec3(1.0f);
};
void BuildImGui(LightColor* light_color);

constexpr u32 kNumPointLights = 4;
struct KDK_ATTR("imgui") PointLight {
    GENERATE_ENTITY(PointLight)

    static ELightType StaticLightType() { return ELightType::Point; }

    LightColor Color = {};

    float MinRadius = 0.1f;
    float MaxRadius = 5.0f;
    float AttenuationConstant = 0.3f;
    float AttenuationLinear = 0.09f;
    float AttenuationQuadratic = 0.032f;

    // RenderState.
    Vec3 RS_ViewPosition = {};
};
void BuildImGui(PointLight* pl);
void Draw(const PointLight& pl, const Shader& shader, const Mesh& mesh, const RenderState& rs);

struct KDK_ATTR("imgui") DirectionalLight {
    GENERATE_ENTITY(DirectionalLight)

    static ELightType StaticLightType() { return ELightType::Directional; }

    Vec3 Direction = Vec3(0);
    LightColor Color = {};

    // RenderState.
    Vec3 RS_ViewDirection = {};
};
void BuildImGui(DirectionalLight* dl);

struct KDK_ATTR("imgui") Spotlight {
    GENERATE_ENTITY(Spotlight)

    static ELightType StaticLightType() { return ELightType::Spotlight; }

    Vec3 Target = Vec3(0);
    LightColor Color = {};

    float MinCutoffDistance = 1.0f;
    float MaxCutoffDistance = 1.5f;
    float InnerRadiusDeg = 12.5f;
    float OuterRadiusDeg = 15.0f;

    // RenderState.
    Vec3 RS_ViewPosition = {};
    Vec3 RS_ViewDirection = {};
    float RS_InnerRadiusCos = 0;
    float RS_OuterRadiusCos = 0;
};
void Recalculate(Spotlight* sl);
void BuildImgui(Spotlight* sl);
Vec3 GetDirection(const Spotlight& sl);
struct KDK_ATTR("imgui") Light {
    ELightType LightType = ELightType::Invalid;
    union {
        PointLight PointLight;
        DirectionalLight DirectionalLight;
        Spotlight Spotlight;
    };
};
Transform& GetTransform(Light* light);

}  // namespace kdk

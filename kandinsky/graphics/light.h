#pragma once

#include <kandinsky/core/defines.h>
#include <kandinsky/core/math.h>
#include <kandinsky/entity.h>
#include <kandinsky/core/serde.h>

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
void Serialize(SerdeArchive* sa, LightColor& lc);

constexpr u32 kNumPointLights = 4;
struct KDK_ATTR("imgui") PointLightComponent {
    GENERATE_COMPONENT(PointLight);

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
void BuildImGui(PointLightComponent* pl);
void BuildGizmos(PlatformState* ps, PointLightComponent* pl);
void Serialize(SerdeArchive* sa, PointLightComponent& pl);
// void Draw(const PointLightComponent& pl,
//           const Shader& shader,
//           const Mesh& mesh,
//           const RenderState& rs);

struct KDK_ATTR("imgui") DirectionalLightComponent {
    GENERATE_COMPONENT(DirectionalLight);

    static ELightType StaticLightType() { return ELightType::Directional; }

    Vec3 Direction = Vec3(0);
    LightColor Color = {};

    // RenderState.
    Vec3 RS_ViewDirection = {};
};
void BuildImGui(DirectionalLightComponent* dl);
void Serialize(SerdeArchive* sa, DirectionalLightComponent& dl);

struct KDK_ATTR("imgui") SpotlightComponent {
    GENERATE_COMPONENT(Spotlight)

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
void BuildImGui(SpotlightComponent* sl);
void Serialize(SerdeArchive* sa, SpotlightComponent& sl);

void Recalculate(SpotlightComponent* sl);
Vec3 GetDirection(const SpotlightComponent& sl);

struct KDK_ATTR("imgui") Light {
    ELightType LightType = ELightType::Invalid;
    union {
        PointLightComponent* PointLight;
        DirectionalLightComponent* DirectionalLight;
        SpotlightComponent* Spotlight;
    };
};
Transform& GetTransform(Light* light);
void BuildImGui(Light* light);

}  // namespace kdk

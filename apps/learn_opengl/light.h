#pragma once

#include <kandinsky/defines.h>
#include <kandinsky/math.h>

namespace kdk {

struct Mesh;
struct PlatformState;
struct Shader;
struct RenderState;

enum class ELightType : u8 {
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
struct PointLight {
    Vec3 Position = Vec3(0);
    LightColor Color = {};

    float MinRadius = 0.1f;
    float MaxRadius = 5.0f;
    float AttenuationConstant = 0.3f;
    float AttenuationLinear = 0.09f;
    float AttenuationQuadratic = 0.032f;

    struct RenderState {
        const PointLight* PL = nullptr;
        Vec3 ViewPosition = {};
    };
};
void BuildImGui(PointLight* pl);
void Draw(const PointLight& pl, const Shader& shader, const Mesh& mesh, const RenderState& rs);

struct DirectionalLight {
    Vec3 Direction = Vec3(0);
    LightColor Color = {};

    struct RenderState {
        const DirectionalLight* DL = nullptr;
        Vec3 ViewDirection = {};
    };
};
void BuildImGui(DirectionalLight* dl);

struct Spotlight {
    Vec3 Position = Vec3(0);
    Vec3 Target = Vec3(0);
    LightColor Color = {};

    float MinCutoffDistance = 1.0f;
    float MaxCutoffDistance = 1.5f;
    float InnerRadiusDeg = 12.5f;
    float OuterRadiusDeg = 15.0f;

    struct RenderState {
        const Spotlight* SL = nullptr;
        Vec3 ViewPosition = {};
        Vec3 ViewDirection = {};
        float InnerRadiusCos = 0;
        float OuterRadiusCos = 0;
    };
};
void Recalculate(Spotlight* sl);
void BuildImgui(Spotlight* sl);
inline Vec3 GetDirection(const Spotlight& sl) { return Normalize(sl.Target - sl.Position); }

struct RenderState {
    Mat4* MatView = nullptr;
    Mat4* MatProj = nullptr;
    Mat4* MatViewProj = nullptr;
    Mat4* MatNormal = nullptr;

    DirectionalLight::RenderState DirectionalLight = {};
    PointLight::RenderState PointLights[kNumPointLights] = {};
    Spotlight::RenderState Spotlight = {};

    float Seconds = 0;
};
inline Vec3 ToView(const RenderState& rs, const Vec3 v) { return *rs.MatView * Vec4(v, 0.0f); }

void DrawMesh(const Mesh& mesh, const Shader& shader, const RenderState& rs);

}  // namespace kdk

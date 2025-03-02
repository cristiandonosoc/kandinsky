#pragma once

#include <kandinsky/math.h>

#include <kandinsky/graphics/light.h>

namespace kdk {

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

void SetUniforms(const RenderState& rs, const Shader& shader);

} // namespace kdk

#pragma once

#include <kandinsky/math.h>

#include <kandinsky/graphics/light.h>

namespace kdk {

struct RenderState {
	Vec3 CameraPosition = {};

    Mat4 M_View = {};
    Mat4 M_Proj = {};
    Mat4 M_ViewProj = {};

	Mat4 M_Model = {};
	Mat4 M_ViewModel = {};
    Mat4 M_Normal = {};

    DirectionalLight::RenderState DirectionalLight = {};
    PointLight::RenderState PointLights[kNumPointLights] = {};
    Spotlight::RenderState Spotlight = {};

    float Seconds = 0;
};

// Requires M_View to be already set!
void ChangeModelMatrix(RenderState* rs, const Mat4& mmodel);

inline Vec3 ToView(const RenderState& rs, const Vec3 v) { return rs.M_View * Vec4(v, 0.0f); }

void SetUniforms(const RenderState& rs, const Shader& shader);

} // namespace kdk

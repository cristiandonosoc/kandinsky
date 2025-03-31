#pragma once

#include <kandinsky/math.h>

#include <kandinsky/graphics/light.h>

#include <span>

namespace kdk {

struct Camera;

struct RenderState {
    Vec3 CameraPosition = {};

    Mat4 M_View = {};
    Mat4 M_Proj = {};
    Mat4 M_ViewProj = {};

    Mat4 M_Model = {};
    Mat4 M_ViewModel = {};
    Mat4 M_Normal = {};

    Span<Light> Lights = {};

    float Seconds = 0;
};

void SetCamera(RenderState* rs, const Camera& camera);
// This will change the RS_* fields in the lights.
void SetLights(RenderState* rs, Span<Light> lights);

// Requires M_View to be already set!
void ChangeModelMatrix(RenderState* rs, const Mat4& mmodel);

inline Vec3 ToView(const RenderState& rs, const Vec3 v) { return rs.M_View * Vec4(v, 0.0f); }

void SetUniforms(const RenderState& rs, const Shader& shader);

}  // namespace kdk

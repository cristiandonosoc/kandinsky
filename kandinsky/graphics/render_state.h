#pragma once

#include <kandinsky/core/math.h>
#include <kandinsky/graphics/light.h>

#include <span>

namespace kdk {

struct Camera;

struct RenderStateOptions {
    bool IsUsingDebugCamera : 1 = false;
};

struct RenderState {
    Vec3 CameraPosition = {};

    Mat4 M_View = {};
    Mat4 M_Proj = {};
    Mat4 M_ViewProj = {};

    Mat4 M_Model = {};
    Mat4 M_ViewModel = {};
    Mat4 M_ProjViewModel = {};
    Mat4 M_Normal = {};

    Vec3 CameraUp_World = {};
    Vec3 CameraRight_World = {};

    std::span<Light> Lights = {};

    float Seconds = 0;
    Vec2 MousePositionGL = {};
    EntityID EntityID = {};

    RenderStateOptions Options = {};
};

void SetPlatformState(RenderState* rs, const PlatformState& ps);
void SetCamera(RenderState* rs, const Camera& camera);
// This will change the RS_* fields in the lights.
void SetLights(RenderState* rs, std::span<Light> lights);

// Requires M_View to be already set!
void ChangeModelMatrix(RenderState* rs, const Mat4& mmodel);

inline Vec3 ToView(const RenderState& rs, const Vec3 v) { return rs.M_View * Vec4(v, 0.0f); }

void SetBaseUniforms(const RenderState& rs, const Shader& shader);
void SetUniforms(const RenderState& rs, const Shader& shader);
void SetEntity(RenderState* rs, EntityID id);

// EntityPicker ------------------------------------------------------------------------------------

struct EntityPicker {
    u32 SSBO = 0;
};

void Init(EntityPicker* ep);
void StartFrame(EntityPicker* ep);
EntityID EndFrame(EntityPicker* ep);

}  // namespace kdk

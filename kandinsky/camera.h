#pragma once

#include <kandinsky/core/color.h>
#include <kandinsky/core/math.h>

namespace kdk {

struct Entity;
struct PlatformState;

enum class ECameraType : u8 {
    Invalid = 0,
    Free,
    Target,
};

enum class ECameraProjectionType : u8 {
    Invalid = 0,
    Perspective,
    Ortho,
};

struct KDK_ATTR("imgui") Camera {
    Vec3 Position = {};
    Vec3 Front = {};
    Vec3 Up = Vec3(0, 1, 0);
    Vec3 Right = {};

    Vec2 WindowSize = {};

    float MovementSpeed = 2.5f;
    float MouseSensitivity = 0.1f;

    ECameraType CameraType = ECameraType::Free;
    union {
        struct {
            // Euler angles (in radians).
            float Yaw = 0;
            float Pitch = 0;
        } FreeCamera;

        struct {
            Vec3 Target = Vec3(0);
            float Distance = 30.0f;
            float XZAngleDeg = 45.0f;
            float YAngleDeg = 30.f;
        } TargetCamera;
    };

    ECameraProjectionType ProjectionType = ECameraProjectionType::Perspective;
    union {
        struct {
            float AngleDeg = 45.0f;
            float Near = 0.1f;
            float Far = 100.0f;
        } PerspectiveData;

        struct {
            float Zoom = 7.5f;
            float Near = 0.1f;
            float Far = 100.0f;
        } OrthoData;
    };

    // Cached Values.
    Mat4 M_View = Mat4(1.0f);
    Mat4 M_Proj = Mat4(1.0f);
    Mat4 M_ViewProj = Mat4(1.0f);
    Mat4 M_InverseView = Mat4(1.0f);
    Mat4 M_InverseProj = Mat4(1.0f);
};

inline bool IsValid(const Camera& camera) { return camera.CameraType != ECameraType::Invalid; }

void BuildImGui(Camera* camera, u32 image_texture = NULL);
void DrawDebug(PlatformState* ps, const Camera& camera, Color32 color);

void Update(PlatformState* ps, Camera* camera, double dt);
void Recalculate(Camera* camera);

void SetupDebugCamera(const Camera& main_camera, Camera* debug_camera);

void SetTarget(Camera* camera, const Vec3& target);
void SetTarget(Camera* camera, const Entity& entity);

// Returns (world pos, direction).
std::pair<Vec3, Vec3> GetWorldRay(const Camera& camera, Vec2 screen_pos);

}  // namespace kdk

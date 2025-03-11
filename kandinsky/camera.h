#pragma once

#include <kandinsky/math.h>

namespace kdk {

struct PlatformState;

enum class ECameraType : u8 {
    Invalid = 0,
    Free,
    Target,
};

struct Camera {
    ECameraType CameraType = ECameraType::Free;

    Vec3 Position = {};
    Vec3 Front = {};
    Vec3 Up = Vec3(0, 1, 0);
    Vec3 Right = {};

    union {
        struct {
            // Euler angles (in radians).
            float Yaw = 0;
            float Pitch = 0;

            float MovementSpeed = 2.5f;
            float MouseSensitivity = 0.1f;
        } FreeCamera;

        struct {
            Vec3 Target = Vec3(0);
        } TargetCamera;
    };

    // Cached Values.
    Mat4 M_View = Mat4(1.0f);
    Mat4 M_Proj = Mat4(1.0f);
    Mat4 M_ViewProj = Mat4(1.0f);
};

void SetProjection(Camera* camera, const Mat4& mproj);

void Update(PlatformState* ps, Camera* camera, double dt);

}  // namespace kdk

#pragma once

#include <kandinsky/entity.h>
#include <kandinsky/math.h>

namespace kdk {

struct PlatformState;

enum class ECameraType : u8 {
    Invalid = 0,
    Free,
    Target,
};

struct Camera {
    GENERATE_ENTITY(Camera);

    ECameraType CameraType = ECameraType::Free;

    Vec3 Position = {};
    Vec3 Front = {};
    Vec3 Up = Vec3(0, 1, 0);
    Vec3 Right = {};

    float MovementSpeed = 2.5f;
    float MouseSensitivity = 0.1f;

    union {
        struct {
            // Euler angles (in radians).
            float Yaw = 0;
            float Pitch = 0;

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

inline bool IsValid(const Camera& camera) { return camera.CameraType != ECameraType::Invalid; }

void BuildImgui(Camera* camera);

void SetProjection(Camera* camera, const Mat4& mproj);

void Update(PlatformState* ps, Camera* camera, double dt);
void Recalculate(Camera* camera);

void SetupDebugCamera(const Camera& main_camera, Camera* debug_camera);

}  // namespace kdk

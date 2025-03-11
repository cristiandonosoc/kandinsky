#include <kandinsky/camera.h>

#include <kandinsky/input.h>
#include <kandinsky/platform.h>

namespace kdk {

void SetProjection(Camera* camera, const Mat4& mproj) {
    camera->M_Proj = mproj;
    camera->M_ViewProj = mproj * camera->M_View;
}

namespace camera_private {

void UpdateFreeCamera(PlatformState* ps, Camera* camera, double dt) {
    constexpr float kMaxPitch = ToRadians(89.0f);

    if (MOUSE_DOWN(ps, MIDDLE)) {
        Vec2 offset = ps->InputState.MouseMove * camera->FreeCamera.MouseSensitivity;

        camera->FreeCamera.Yaw += ToRadians(offset.x);
        camera->FreeCamera.Yaw = FMod(camera->FreeCamera.Yaw, ToRadians(360.0f));

        camera->FreeCamera.Pitch -= ToRadians(offset.y);
        camera->FreeCamera.Pitch = Clamp(camera->FreeCamera.Pitch, -kMaxPitch, kMaxPitch);
    }

    Vec3 dir;
    dir.x = cos(camera->FreeCamera.Yaw) * cos(camera->FreeCamera.Pitch);
    dir.y = sin(camera->FreeCamera.Pitch);
    dir.z = sin(camera->FreeCamera.Yaw) * cos(camera->FreeCamera.Pitch);
    camera->Front = Normalize(dir);

    camera->Right = Normalize(Cross(camera->Front, Vec3(0.0f, 1.0f, 0.0f)));
    camera->Up = Normalize(Cross(camera->Right, camera->Front));

    float speed = camera->FreeCamera.MovementSpeed * (float)dt;
    if (KEY_DOWN(ps, W)) {
        camera->Position += speed * camera->Front;
    }
    if (KEY_DOWN(ps, S)) {
        camera->Position -= speed * camera->Front;
    }
    if (KEY_DOWN(ps, A)) {
        camera->Position -= speed * camera->Right;
    }
    if (KEY_DOWN(ps, D)) {
        camera->Position += speed * camera->Right;
    }

    camera->M_View = LookAt(camera->Position, camera->Position + camera->Front, camera->Up);
}

void UpdateTargetCamera(PlatformState* ps, Camera* camera, double dt) {
    float speed = camera->FreeCamera.MovementSpeed * (float)dt;

    Vec3 f = Normalize(Vec3(camera->Front.x, 0, camera->Front.z));
    Vec3 r = Normalize(Vec3(camera->Right.x, 0, camera->Right.z));

    if (KEY_DOWN(ps, W)) {
        camera->Position += speed * f;
    }
    if (KEY_DOWN(ps, S)) {
        camera->Position -= speed * f;
    }
    if (KEY_DOWN(ps, A)) {
        camera->Position -= speed * r;
    }
    if (KEY_DOWN(ps, D)) {
        camera->Position += speed * r;
    }

    camera->M_View = LookAtTarget(camera->Position, camera->TargetCamera.Target, camera->Up);
}

}  // namespace camera_private

void Update(PlatformState* ps, Camera* camera, double dt) {
    using namespace camera_private;

    switch (camera->CameraType) {
        case ECameraType::Invalid: ASSERT(false); return;
        case ECameraType::Free: UpdateFreeCamera(ps, camera, dt); break;
        case ECameraType::Target: UpdateTargetCamera(ps, camera, dt); break;
    }

    // float aspect_ratio = (float)(ps->Window.Width) / (float)(ps->Window.Height);
    // camera->Proj = Perspective(ToRadians(45.0f), aspect_ratio, 0.1f, 100.0f);
    // float bound = 5;
    // camera->Proj = Ortho(bound * aspect_ratio, bound, 0.1f, 100.f);

    camera->M_ViewProj = camera->M_Proj * camera->M_View;
}

}  // namespace kdk

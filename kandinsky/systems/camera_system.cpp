#include <kandinsky/systems/camera_system.h>

#include <kandinsky/debug.h>
#include <kandinsky/platform.h>

namespace kdk {

namespace camera_system_private {}  // namespace camera_system_private

void Start(CameraSystem* cs) {
    ASSERT(!cs->Camera);
    cs->Camera = &cs->GetPlatformState()->GameCamera;

    SetTarget(cs->Camera, *GetEntity(cs->GetEntityManager(), cs->GetGameMode()->Base));
}

void Stop(CameraSystem* cs) { cs->Camera = nullptr; }

void Update(CameraSystem* cs, float dt) {
    auto* ps = cs->GetPlatformState();
    auto* camera = cs->Camera;

    float speed = camera->MovementSpeed * (float)dt;

    // Vec3 dir = Normalize(camera->TargetCamera.Target - camera->Position);
    // float yAngleDeg = ToDegrees(Asin(dir.y));
    // float xzAngleDeg = ToDegrees(Atan2(dir.z, dir.x));

    Vec3 front = Normalize(Vec3(camera->Front.x, 0, camera->Front.z));
    Vec3 right = Normalize(Vec3(camera->Right.x, 0, camera->Right.z));

    Vec3 target = camera->TargetCamera.Target;
    if (KEY_DOWN(ps, W)) {
        target += speed * front;
    }
    if (KEY_DOWN(ps, S)) {
        target -= speed * front;
    }
    if (KEY_DOWN(ps, A)) {
        target -= speed * right;
    }
    if (KEY_DOWN(ps, D)) {
        target += speed * right;
    }

    SetTarget(camera, target);
    Debug::DrawSphere(ps, camera->TargetCamera.Target, 0.3f, 8, Color32::Cyan);
}

}  // namespace kdk

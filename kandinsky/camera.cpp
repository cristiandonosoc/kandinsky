#include <imgui.h>
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
        Vec2 offset = ps->InputState.MouseMove * camera->MouseSensitivity;

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

    float speed = camera->MovementSpeed * (float)dt;
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
    float speed = camera->MovementSpeed * (float)dt;

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

void BuildImgui(Camera* camera) {
    // Camera type selection
    int camera_type = (int)camera->CameraType;
    ImGui::Text("Camera Type");
    ImGui::RadioButton("Free", &camera_type, (int)ECameraType::Free);
    ImGui::SameLine();
    ImGui::RadioButton("Target", &camera_type, (int)ECameraType::Target);
    camera->CameraType = (ECameraType)camera_type;

    // Position for both camera types
    ImGui::DragFloat3("Position", &camera->Position.x, 0.1f);
    ImGui::InputFloat3("Front", &camera->Front.x, "%.2f", ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat3("Up", &camera->Up.x, "%.2f", ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat3("Right", &camera->Right.x, "%.2f", ImGuiInputTextFlags_ReadOnly);

    ImGui::Separator();

    // Type-specific settings
    if (camera->CameraType == ECameraType::Free) {
        if (ImGui::CollapsingHeader("Free Camera Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Convert radians to degrees for UI
            float yawDegrees = glm::degrees(camera->FreeCamera.Yaw);
            float pitchDegrees = glm::degrees(camera->FreeCamera.Pitch);

            // Adjust angles
            if (ImGui::SliderFloat("Yaw", &yawDegrees, 0.0f, 359.9f)) {
                camera->FreeCamera.Yaw = ToRadians(yawDegrees);
            }

            if (ImGui::SliderFloat("Pitch", &pitchDegrees, -89.0f, 89.0f)) {
                camera->FreeCamera.Pitch = ToRadians(pitchDegrees);
            }

            // Movement and sensitivity settings
            ImGui::SliderFloat("Movement Speed", &camera->MovementSpeed, 0.5f, 10.0f);
            ImGui::SliderFloat("Mouse Sensitivity", &camera->MouseSensitivity, 0.01f, 1.0f);
        }
    } else if (camera->CameraType == ECameraType::Target) {
        if (ImGui::CollapsingHeader("Target Camera Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Target position
            ImGui::DragFloat3("Target", &camera->TargetCamera.Target.x, 0.1f);

            // Calculate and display distance to target
            float distanceToTarget = glm::length(camera->Position - camera->TargetCamera.Target);
            ImGui::Text("Distance to Target: %.2f", distanceToTarget);
        }
    }

    ImGui::Separator();

    // Display matrix information
    if (ImGui::CollapsingHeader("Matrices")) {
        if (ImGui::TreeNode("View Matrix")) {
            for (int row = 0; row < 4; row++) {
                ImGui::Text("%.2f %.2f %.2f %.2f",
                            camera->M_View[0][row],
                            camera->M_View[1][row],
                            camera->M_View[2][row],
                            camera->M_View[3][row]);
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Projection Matrix")) {
            for (int row = 0; row < 4; row++) {
                ImGui::Text("%.2f %.2f %.2f %.2f",
                            camera->M_Proj[0][row],
                            camera->M_Proj[1][row],
                            camera->M_Proj[2][row],
                            camera->M_Proj[3][row]);
            }
            ImGui::TreePop();
        }
    }
}

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

void Recalculate(Camera* camera) {
    if (camera->CameraType == ECameraType::Free) {
        Vec3 dir;
        dir.x = cos(camera->FreeCamera.Yaw) * cos(camera->FreeCamera.Pitch);
        dir.y = sin(camera->FreeCamera.Pitch);
        dir.z = sin(camera->FreeCamera.Yaw) * cos(camera->FreeCamera.Pitch);
        camera->Front = Normalize(dir);

        camera->Right = Normalize(Cross(camera->Front, Vec3(0.0f, 1.0f, 0.0f)));
        camera->Up = Normalize(Cross(camera->Right, camera->Front));

        camera->M_View = LookAt(camera->Position, camera->Position + camera->Front, camera->Up);
    } else if (camera->CameraType == ECameraType::Target) {
        camera->M_View = LookAtTarget(camera->Position, camera->TargetCamera.Target, camera->Up);
    }

    camera->M_ViewProj = camera->M_Proj * camera->M_View;
}

void SetupDebugCamera(const Camera& main_camera, Camera* debug_camera) {
    // We only do this if the debug camera has not been initialized.
    // Proxy is checking if it is on the origin.
    if (!IsZero(debug_camera->Position)) {
        return;
    }

    debug_camera->Position = main_camera.Position;
    debug_camera->Front = main_camera.Front;
    debug_camera->Up = main_camera.Up;
    debug_camera->Right = main_camera.Right;
}

}  // namespace kdk

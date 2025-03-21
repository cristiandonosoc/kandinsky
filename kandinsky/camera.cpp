#include <imgui.h>
#include <kandinsky/camera.h>

#include <kandinsky/debug.h>
#include <kandinsky/input.h>
#include <kandinsky/platform.h>

namespace kdk {

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
        camera->TargetCamera.Target += speed * f;
    }
    if (KEY_DOWN(ps, S)) {
        camera->TargetCamera.Target -= speed * f;
    }
    if (KEY_DOWN(ps, A)) {
        camera->TargetCamera.Target -= speed * r;
    }
    if (KEY_DOWN(ps, D)) {
        camera->TargetCamera.Target += speed * r;
    }

    Recalculate(camera);
}

void SetProjection(Camera* camera, const Mat4& mproj) {
    camera->M_Proj = mproj;
    camera->M_ViewProj = mproj * camera->M_View;
}

}  // namespace camera_private

void BuildImgui(Camera* camera, u32 image_texture) {
    // Camera type selection
    int camera_type = (int)camera->CameraType;
    ImGui::Text("Camera Type");
    ImGui::RadioButton("Free", &camera_type, (int)ECameraType::Free);
    ImGui::SameLine();
    ImGui::RadioButton("Target", &camera_type, (int)ECameraType::Target);
    camera->CameraType = (ECameraType)camera_type;

    ImGui::DragFloat3("Position", &camera->Position.x, 0.1f);
    ImGui::InputFloat3("Front", &camera->Front.x, "%.2f", ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat3("Up", &camera->Up.x, "%.2f", ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat3("Right", &camera->Right.x, "%.2f", ImGuiInputTextFlags_ReadOnly);

    ImGui::InputFloat("Movement Speed", &camera->MovementSpeed, 0.1f, 1.0f);
    ImGui::InputFloat("Mouse Sensitivity", &camera->MouseSensitivity, 0.01f, 0.1f);

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
            ImGui::DragFloat("Distance", &camera->TargetCamera.Distance, 0.1f);
            ImGui::DragFloat("XZ Angle", &camera->TargetCamera.XZAngleDeg, 0.1f);
            ImGui::DragFloat("Y Angle", &camera->TargetCamera.YAngleDeg, 0.1f);

            // Calculate and display distance to target
            float distanceToTarget = Distance(camera->Position, camera->TargetCamera.Target);
            ImGui::Text("Distance to Target: %.2f", distanceToTarget);
        }
    }

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Projection Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        int projection_type = (int)camera->ProjectionType;
        ImGui::RadioButton("Pespective", &projection_type, (int)ECameraProjectionType::Perspective);
        ImGui::SameLine();
        ImGui::RadioButton("Ortho", &projection_type, (int)ECameraProjectionType::Ortho);
        camera->ProjectionType = (ECameraProjectionType)projection_type;

        if (camera->ProjectionType == ECameraProjectionType::Perspective) {
            ImGui::SliderFloat("Angle", &camera->PerspectiveData.AngleDeg, 1.0f, 179.0f);
            ImGui::DragFloat("Near", &camera->PerspectiveData.Near, 0.1f);
            ImGui::DragFloat("Far", &camera->PerspectiveData.Far, 0.1f);
        } else if (camera->ProjectionType == ECameraProjectionType::Ortho) {
            ImGui::DragFloat("Zoom", &camera->OrthoData.Zoom, 0.1f);
            ImGui::DragFloat("Near", &camera->OrthoData.Near, 0.1f);
            ImGui::DragFloat("Far", &camera->OrthoData.Far, 0.1f);
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

    if (image_texture != NULL) {
        ImGui::Image((ImTextureID)image_texture, {640, 480}, {0, 1}, {1, 0});
    }
}

void DrawDebug(PlatformState* ps, const Camera& camera, Color32 color) {
    Debug::DrawFrustum(ps, camera.M_ViewProj, color, 3);

    if (camera.CameraType == ECameraType::Target) {
        Debug::DrawSphere(ps, camera.TargetCamera.Target, 0.5f, 16, color);
        Debug::DrawArrow(ps, camera.Position, camera.TargetCamera.Target, color);
    }
}

void Update(PlatformState* ps, Camera* camera, double dt) {
    using namespace camera_private;

    camera->WindowSize = {ps->Window.Width, ps->Window.Height};

    switch (camera->CameraType) {
        case ECameraType::Invalid: ASSERT(false); return;
        case ECameraType::Free: UpdateFreeCamera(ps, camera, dt); break;
        case ECameraType::Target: UpdateTargetCamera(ps, camera, dt); break;
    }

    Recalculate(camera);
}

void Recalculate(Camera* camera) {
    using namespace camera_private;

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
        Vec3 dir = {};
        dir.x = Cos(ToRadians(camera->TargetCamera.XZAngleDeg));
        dir.y = Sin(ToRadians(camera->TargetCamera.YAngleDeg));
        dir.z = Sin(ToRadians(camera->TargetCamera.XZAngleDeg));
        dir = Normalize(dir);

        camera->Position = camera->TargetCamera.Target + dir * camera->TargetCamera.Distance;

        camera->Front = -dir;
        camera->Right = Normalize(Cross(camera->Front, Vec3(0.0f, 1.0f, 0.0f)));
        camera->Up = Normalize(Cross(camera->Right, camera->Front));

        camera->M_View = LookAt(camera->Position, camera->Position + camera->Front, camera->Up);
    }

    float aspect_ratio = camera->WindowSize.x / camera->WindowSize.y;
    switch (camera->ProjectionType) {
        case ECameraProjectionType::Invalid: ASSERT(false); break;
        case ECameraProjectionType::Perspective:
            SetProjection(camera,
                          Perspective(ToRadians(camera->PerspectiveData.AngleDeg),
                                      aspect_ratio,
                                      camera->PerspectiveData.Near,
                                      camera->PerspectiveData.Far));
            break;
        case ECameraProjectionType::Ortho:
            SetProjection(camera,
                          Ortho(camera->OrthoData.Zoom * aspect_ratio,
                                camera->OrthoData.Zoom,
                                camera->OrthoData.Near,
                                camera->OrthoData.Far));
            break;
    }

    camera->M_ViewProj = camera->M_Proj * camera->M_View;
    camera->M_InverseView = Inverse(camera->M_View);
    camera->M_InverseProj = Inverse(camera->M_Proj);
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

std::pair<Vec3, Vec3> GetWorldRay(const Camera& camera, Vec2 screen_pos) {
    // Convert coords to the NDC (-1 to 1) space.
    float ndc_x = (2.0f * screen_pos.x) / camera.WindowSize.x - 1.0f;
    float ndc_y = 1.0f - (2.0f * screen_pos.y) / camera.WindowSize.y;

    if (camera.ProjectionType == ECameraProjectionType::Ortho) {
        if (screen_pos.y > camera.WindowSize.y / 2) {
            screen_pos.y = -screen_pos.y;
        }
    }

    // Create NDC position with Z at the near plane. (OpenGL uses left-handed coordinate system).
    Vec4 ndc_pos = Vec4(ndc_x, ndc_y, -1.0f, 1.0f);

    switch (camera.ProjectionType) {
        case ECameraProjectionType::Invalid: ASSERT(false); return {};
        case ECameraProjectionType::Perspective: {
            // Convert from clip space to view space. Reset the Z and W.
            Vec4 view_pos = camera.M_InverseProj * ndc_pos;
            view_pos.z = -1.0f;
            view_pos.w = 0.0f;

            Vec3 world_pos = Vec3(camera.M_InverseView * view_pos);
            Vec3 world_dir = Normalize(world_pos);
            return {camera.Position, world_dir};
        }
        case ECameraProjectionType::Ortho: {
            // Convert from clip space to view space. Reset the Z and W.
            Vec4 view_pos = camera.M_InverseProj * ndc_pos;
            view_pos.z = -1.0f;
            view_pos.w = 1.0f;

            Vec3 world_pos = Vec3(camera.M_InverseView * view_pos);
            return {world_pos, camera.Front};
        }
    }

    ASSERT(false);
    return {};
}

}  // namespace kdk

#include <imgui.h>
#include <kandinsky/camera.h>

#include <kandinsky/debug.h>
#include <kandinsky/input.h>
#include <kandinsky/platform.h>

namespace kdk {

namespace camera_private {

void UpdateFreeCamera(PlatformState* ps, Camera* camera, double dt) {
    constexpr float kMaxPitch = ToRadians(89.0f);

    if (MOUSE_DOWN(ps, RIGHT)) {
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

void SetProjection(Camera* camera, const Mat4& mproj) {
    camera->M_Proj = mproj;
    camera->M_ViewProj = mproj * camera->M_View;
}

void UpdateTargetCamera(PlatformState* ps, Camera* camera, double dt) {
    constexpr float kMaxPitch = 89.0f;

    float speed = camera->MovementSpeed * (float)dt;

    Vec3 dir = Normalize(camera->TargetCamera.Target - camera->Position);
    float yAngleDeg = ToDegrees(Asin(dir.y));
    float xzAngleDeg = ToDegrees(Atan2(dir.z, dir.x));

    Vec3 front = Normalize(Vec3(camera->Front.x, 0, camera->Front.z));
    Vec3 right = Normalize(Vec3(camera->Right.x, 0, camera->Right.z));

    // Handle mouse movement for looking around (similar to free camera).
    if (MOUSE_DOWN(ps, MIDDLE)) {
        Vec2 offset = ps->InputState.MouseMove * camera->MouseSensitivity;

        // Update the angles based on mouse movement.
        xzAngleDeg += offset.x;
        xzAngleDeg = FMod(xzAngleDeg, 360.0f);

        yAngleDeg -= offset.y;
        yAngleDeg = Clamp(yAngleDeg, -kMaxPitch, kMaxPitch);

        // Convert angles to radians.
        float xz_angle_rad = ToRadians(xzAngleDeg);
        float y_angle_rad = ToRadians(yAngleDeg);

        // Proper spherical coordinates.
        Vec3 result_dir = {};
        result_dir.x = cos(y_angle_rad) * cos(xz_angle_rad);
        result_dir.y = sin(y_angle_rad);
        result_dir.z = cos(y_angle_rad) * sin(xz_angle_rad);
        result_dir = Normalize(result_dir);

        camera->Position = camera->TargetCamera.Target - result_dir * camera->TargetCamera.Distance;
    }

    if (KEY_DOWN(ps, W)) {
        camera->TargetCamera.Target += speed * front;
    }
    if (KEY_DOWN(ps, S)) {
        camera->TargetCamera.Target -= speed * front;
    }
    if (KEY_DOWN(ps, A)) {
        camera->TargetCamera.Target -= speed * right;
    }
    if (KEY_DOWN(ps, D)) {
        camera->TargetCamera.Target += speed * right;
    }
}

void ConvertFromTargetToFree(Camera* camera) {
    ASSERT(camera->CameraType == ECameraType::Target);

    Vec3 dir = Normalize(camera->TargetCamera.Target - camera->Position);
    camera->FreeCamera.Pitch = asinf(dir.y);
    camera->FreeCamera.Yaw = atan2f(dir.z, dir.x);
    camera->CameraType = ECameraType::Free;
}

void ConvertFromFreeToTarget(Camera* camera) {
    ASSERT(camera->CameraType == ECameraType::Free);

    Vec3 dir = camera->Front;
    camera->TargetCamera.Target = camera->Position + dir * camera->TargetCamera.Distance;
    camera->CameraType = ECameraType::Target;
}

}  // namespace camera_private

void Recalculate(Camera* camera, double dt) {
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
        Vec3 dir = Normalize(camera->TargetCamera.Target - camera->Position);

        Vec3 want_pos = camera->TargetCamera.Target - dir * camera->TargetCamera.Distance;
        camera->Position = Decay(camera->Position, want_pos, 0.5f, (float)dt);

        camera->Front = dir;
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
        case ECameraProjectionType::Orthogonal:
            SetProjection(camera,
                          Ortho(camera->OrthogonalData.Zoom * aspect_ratio,
                                camera->OrthogonalData.Zoom,
                                camera->OrthogonalData.Near,
                                camera->OrthogonalData.Far));
            break;
    }

    camera->M_ViewProj = camera->M_Proj * camera->M_View;
    camera->M_InverseView = Inverse(camera->M_View);
    camera->M_InverseProj = Inverse(camera->M_Proj);
}

void BuildImGui(Camera* camera, u32 image_texture) {
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

            float yAngleDeg = ToDegrees(Asin(camera->Front.y));
            float xzAngleDeg = ToDegrees(Atan2(camera->Front.z, camera->Front.x));

            ImGui::Text("XZ Angle: %.2f", xzAngleDeg);
            ImGui::Text("Y Angle: %.2f", yAngleDeg);

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
        ImGui::RadioButton("Ortho", &projection_type, (int)ECameraProjectionType::Orthogonal);
        camera->ProjectionType = (ECameraProjectionType)projection_type;

        if (camera->ProjectionType == ECameraProjectionType::Perspective) {
            ImGui::SliderFloat("Angle", &camera->PerspectiveData.AngleDeg, 1.0f, 179.0f);
            ImGui::DragFloat("Near", &camera->PerspectiveData.Near, 0.1f);
            ImGui::DragFloat("Far", &camera->PerspectiveData.Far, 0.1f);
        } else if (camera->ProjectionType == ECameraProjectionType::Orthogonal) {
            ImGui::DragFloat("Zoom", &camera->OrthogonalData.Zoom, 0.1f);
            ImGui::DragFloat("Near", &camera->OrthogonalData.Near, 0.1f);
            ImGui::DragFloat("Far", &camera->OrthogonalData.Far, 0.1f);
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

    if (!camera->IsDebugCamera) {
        if (MOUSE_PRESSED(ps, RIGHT)) {
            if (camera->CameraType == ECameraType::Target) {
                ConvertFromTargetToFree(camera);
            }
        } else if (MOUSE_RELEASED(ps, RIGHT)) {
            if (camera->CameraType == ECameraType::Free) {
                ConvertFromFreeToTarget(camera);
            }
        }
    }

    switch (camera->CameraType) {
        case ECameraType::Invalid: ASSERT(false); return;
        case ECameraType::Free: UpdateFreeCamera(ps, camera, dt); break;
        case ECameraType::Target: UpdateTargetCamera(ps, camera, dt); break;
    }

    Recalculate(camera, dt);
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

void SetTarget(Camera* camera, const Vec3& target) {
    if (camera->CameraType == ECameraType::Target) {
        // We have to ensure to keep the direction.
        Vec3 diff = camera->TargetCamera.Target - camera->Position;
        float distance = Length(diff);
        Vec3 dir = Normalize(diff);

        camera->TargetCamera.Target = target;
        camera->Position = target - dir * distance;
    }
}

void SetTarget(Camera* camera, const Entity& entity) {
    SetTarget(camera, entity.Transform.Position);
}

Ray GetWorldRay(const Camera& camera, Vec2 screen_pos) {
    // Convert coords to the NDC (-1 to 1) space.
    float ndc_x = (2.0f * screen_pos.x) / camera.WindowSize.x - 1.0f;
    float ndc_y = 1.0f - (2.0f * screen_pos.y) / camera.WindowSize.y;

    // Convert the point in the NDC near and far plane into view space and take the direction.
    Vec4 near_ndc_pos = Vec4(ndc_x, ndc_y, -1.0f, 1.0f);
    Vec4 near_view_pos = camera.M_InverseProj * near_ndc_pos;
    near_view_pos /= near_view_pos.w;
    Vec3 near_world_pos = Vec3(camera.M_InverseView * near_view_pos);

    Vec4 far_ndc_pos = Vec4(ndc_x, ndc_y, 0.0f, 1.0f);
    Vec4 far_view_pos = camera.M_InverseProj * far_ndc_pos;
    far_view_pos /= far_view_pos.w;
    Vec3 far_world_pos = Vec3(camera.M_InverseView * far_view_pos);

    // The ray starts from the "near" plane.
    Vec3 world_dir = Normalize(far_world_pos - near_world_pos);
    return {near_world_pos, world_dir};
}

Optional<GridRayResult> GetGridRayIntersection(const Camera& camera, const Vec2& mouse_pos) {
    Plane base_plane{
        .Normal = Vec3(0, 1, 0),
    };

    auto [ray_pos, ray_dir] = GetWorldRay(camera, mouse_pos);

    Vec3 intersection = {};
    if (!IntersectPlaneRay(base_plane, ray_pos, ray_dir, &intersection)) {
        return {};
    }

    Vec3 grid_world_location = Round(intersection);
    IVec2 grid_coord = UVec2((i32)grid_world_location.x, (i32)grid_world_location.z);

    GridRayResult result = {
        .IntersectionPoint = intersection,
        .GridWorldLocation = grid_world_location,
        .GridCoord = grid_coord,
    };
    return result;
}

}  // namespace kdk

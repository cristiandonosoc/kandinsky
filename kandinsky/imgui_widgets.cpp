#include <kandinsky/imgui_widgets.h>

namespace kdk {

void BuildImGui(Transform* transform) {
    ImGui::DragFloat3("Position", (float*)&transform->Position, 0.1f);

    Vec3 euler = ToEulerDegrees(transform->Rotation);
    if (ImGui::DragFloat3("Rotation", (float*)&euler, 0.1f, -180.0f, 180.0f, "%.2f")) {
        Vec3 radians = ToRadians(euler);
        transform->Rotation = Quat(radians);
    }

    ImGui::DragFloat3("Scale", (float*)&transform->Scale, 0.1f);
}

}  // namespace kdk

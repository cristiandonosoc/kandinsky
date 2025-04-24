#include <kandinsky/imgui_widgets.h>

namespace kdk {

void BuildImGui(Transform* transform) {
    ImGui::DragFloat3("Position", (float*)&transform->Position, 0.1f);
    ImGui::DragFloat4("Rotation", (float*)&transform->Rotation, 0.1f);
    ImGui::DragFloat3("Scale", (float*)&transform->Scale, 0.1f);
}

}  // namespace kdk

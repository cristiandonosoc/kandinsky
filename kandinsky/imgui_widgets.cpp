#include <kandinsky/imgui_widgets.h>

#include <bitset>

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

void BuildImGui_EntitySignature(EntitySignature signature) {
    std::bitset<32> bits((i32)signature);
    std::string bit_str = bits.to_string();

    // Add spaces every 4 bits for readability
    std::string formatted;
    for (size_t i = 0; i < bit_str.length(); i++) {
        if (i > 0 && i % 4 == 0) {
            formatted += "'";
        }
        formatted += bit_str[i];
    }

    ImGui::Text("Signature: %s (%d)", formatted.c_str(), signature);
}

}  // namespace kdk

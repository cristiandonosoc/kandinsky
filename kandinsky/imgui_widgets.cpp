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

bool ImGui_WarningButton(const char* label, const ImVec2& size) {
    // clang-format off
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));        // Orange-yellow
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.7f, 0.3f, 1.0f)); // Lighter on hover
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.5f, 0.1f, 1.0f));  // Darker when clicked
    // clang-format on

    bool pressed = ImGui::Button(label, size);

    ImGui::PopStyleColor(3);
    return pressed;
}

bool ImGui_DangerButton(const char* label, const ImVec2& size) {
    // clang-format off
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));        // Red
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f)); // Lighter red on hover
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));  // Darker red when clicked
    // clang-format on

    bool pressed = ImGui::Button(label, size);

    ImGui::PopStyleColor(3);
    return pressed;
}

}  // namespace kdk

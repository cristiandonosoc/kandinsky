#include <imgui.h>
#include <kandinsky/gameplay/ui.h>

#include <kandinsky/imgui.h>
#include <kandinsky/platform.h>

namespace kdk {

namespace ui_private {

void TowerButtonsUI(PlatformState* ps) {
    // Define button size
    ImVec2 button_size(200, 80);  // width, height
    constexpr i32 button_count = 1;

    // Calculate window size based on your buttons
    float button_group_width = button_size.x * button_count + ImGui::GetStyle().ItemSpacing.x * 2;
    float button_group_height = button_size.y + ImGui::GetStyle().WindowPadding.y * 2;

    // Center the window
    ImGui::SetNextWindowPos(ImVec2((ps->Window.Width - button_group_width) / 2,
                                   (ps->Window.Height - button_group_height)));
    ImGui::SetNextWindowSize(ImVec2(button_group_width, button_group_height));

    ImGui::Begin("ButtonBar",
                 nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoBackground);
    DEFER { ImGui::End(); };

    ImGui::SetWindowFontScale(2.0f);

    // Or adjust padding for bigger buttons
    // ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 20));
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);  // Full opacity

    DEFER { ImGui::PopStyleVar(); };

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.50f, 0.90f, 1.0f));         // Opaque
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.26f, 0.59f, 0.98f, 1.0f));  // Opaque
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.06f, 0.53f, 0.98f, 1.0f));   // Opaque

    if (ImGui::Button("Place Tower", button_size)) { /* ... */
        StartPlaceBuilding(&ps->GameMode);
    }
    // ImGui::SameLine();
    // if (ImGui::Button("Action 2", button_size)) { /* ... */
    //     SDL_Log("Click!\n");
    // }
    // ImGui::SameLine();
    // if (ImGui::Button("Action 3", button_size)) { /* ... */
    //     SDL_Log("Click!\n");
    // }

    ImGui::PopStyleColor(3);
}

void TextUI(PlatformState* ps) {
    (void)ps;
    // Example text UI
    ImGui::Begin("Info",
                 nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);

    DEFER { ImGui::End(); };

    ImGui::SetWindowFontScale(2.0f);

    ImGui::Text("Welcome to the Tower Defense Game!");
    ImGui::Text("Money: %d", (i32)ps->GameMode.Money);
}

}  // namespace ui_private

void UpdateUI(PlatformState* ps) {
    using namespace ui_private;
    TextUI(ps);
    TowerButtonsUI(ps);
}

void RenderUI(PlatformState* ps) { (void)ps; }

}  // namespace kdk

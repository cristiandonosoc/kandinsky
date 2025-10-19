#include <imgui.h>
#include <kandinsky/gameplay/ui.h>

#include <kandinsky/imgui.h>
#include <kandinsky/platform.h>

namespace kdk {

void UpdateUI(PlatformState* ps) {
    // static constexpr float kButtonBarHeight = 100.0f;

    //     // Create an invisible window at the bottom
    //     ImGui::SetNextWindowPos(ImVec2(0, (float)ps->Window.Height - kButtonBarHeight));
    //     ImGui::SetNextWindowSize(ImVec2((float)ps->Window.Width, kButtonBarHeight));
    //     ImGui::Begin("ButtonBar",
    //                  nullptr,
    //                  ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
    //                  ImGuiWindowFlags_NoMove |
    //                      ImGuiWindowFlags_NoBackground);  // Optional: transparent background
    //     DEFER { ImGui::End(); };

    //     if (ImGui::Button("Action 1")) { /* ... */
    //     }
    //     ImGui::SameLine();
    //     if (ImGui::Button("Action 2")) { /* ... */
    //     }

    // Define button size
    ImVec2 button_size(200, 80);  // width, height

    // Calculate window size based on your buttons
    float button_group_width =
        button_size.x * 3 + ImGui::GetStyle().ItemSpacing.x * 2;  // 3 buttons with spacing
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
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 20));

    DEFER { ImGui::PopStyleVar(); };

    if (ImGui::Button("Action 1", button_size)) { /* ... */
    }
    ImGui::SameLine();
    if (ImGui::Button("Action 2", button_size)) { /* ... */
    }
    ImGui::SameLine();
    if (ImGui::Button("Action 3", button_size)) { /* ... */
    }
}

void RenderUI(PlatformState* ps) { (void)ps; }

}  // namespace kdk

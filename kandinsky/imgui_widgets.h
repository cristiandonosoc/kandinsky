#pragma once

#include <kandinsky/core/math.h>
#include <kandinsky/entity.h>
#include <kandinsky/imgui.h>

namespace kdk {

void BuildImGui(Transform* transform);
void BuildImGui_EntitySignature(EntitySignature signature);

bool ImGui_WarningButton(const char* label, const ImVec2& size = ImVec2(0, 0));
bool ImGui_DangerButton(const char* label, const ImVec2& size = ImVec2(0, 0));

}  // namespace kdk

#pragma once

#include <kandinsky/core/math.h>
#include <kandinsky/entity.h>
#include <kandinsky/imgui.h>

namespace kdk {

void BuildImGui(Transform* transform);
void BuildImGui_EntitySignature(EntitySignature signature);

bool ImGui_WarningButton(const char* label, const ImVec2& size = ImVec2(0, 0));
bool ImGui_DangerButton(const char* label, const ImVec2& size = ImVec2(0, 0));

template <typename T>
    requires std::is_enum_v<T>
T ImGui_EnumCombo(String label, T current_value) {
    FixedVector<const char*, (i32)T::COUNT> values;
    for (i32 i = 0; i < (i32)T::COUNT; i++) {
        T value = (T)i;
        values.Push(ToString(value).Str());
    }

    // We subtract 1 because we don't want to show the "Invalid" option.
    int selected_index = (i32)current_value;
    ImGui::Combo(label.Str(), &selected_index, values.Data, values.Size);

    return (T)selected_index;
}

}  // namespace kdk

#include <kandinsky/experimental/entity_reflection.h>

#include <kandinsky/imgui.h>

namespace kdk {

void BuildImGuiForMetadata(String name, const EntityFieldMetadataHolder& metadata_holder) {
    if (ImGui::BeginTable(name.Str(),
                          5,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY)) {
        // Setup columns
        ImGui::TableSetupColumn("Field Name", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Default Value", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Flags", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupScrollFreeze(0, 1);  // Freeze header row
        ImGui::TableHeadersRow();

        // Display rows
        for (i32 i = 0; i < metadata_holder.FieldCount; ++i) {
            const auto& field = metadata_holder.FieldMetadatas[i];

            ImGui::TableNextRow();

            // Field Name
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(field.FieldName ? field.FieldName : "N/A");

            // Field Type
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(field.FieldType ? field.FieldType : "N/A");

            // Size
            ImGui::TableNextColumn();
            ImGui::Text("%d", field.FieldTypeSize);

            // Default Value
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(field.DefaultValueStr ? field.DefaultValueStr : "N/A");

            // Flags (display as binary or list set bits)
            ImGui::TableNextColumn();
            ImGui::Text("%s", field.Flags.to_string().c_str());

            // Alternative: Show only set flags
            // bool first = true;
            // for (size_t bit = 0; bit < field.Flags.size(); ++bit) {
            //     if (field.Flags.test(bit)) {
            //         if (!first) ImGui::SameLine(); ImGui::TextDisabled("|"); ImGui::SameLine();
            //         ImGui::Text("%zu", bit);
            //         first = false;
            //     }
            // }
            // if (first) ImGui::TextDisabled("(none)");
        }

        ImGui::EndTable();
    }
}

}  // namespace kdk

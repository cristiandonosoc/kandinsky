#include <kandinsky/imgui_widgets.h>

#include <bitset>
#include "kandinsky/core/memory.h"
#include "kandinsky/core/string.h"

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

void ImGui_PushStyleColor(EImGuiStyle style) {
    switch (style) {
        case EImGuiStyle::Ok:
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));    // Green
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));  // Green
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                                  ImVec4(0.3f, 0.9f, 0.3f, 1.0f));  // Lighter green on hover
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                                  ImVec4(0.1f, 0.7f, 0.1f, 1.0f));  // Darker green when clicked
            return;
        case EImGuiStyle::Warning:
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));  // Orange-yellow
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImVec4(0.8f, 0.6f, 0.2f, 1.0f));  // Orange-yellow
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                                  ImVec4(0.9f, 0.7f, 0.3f, 1.0f));  // Lighter on hover
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                                  ImVec4(0.7f, 0.5f, 0.1f, 1.0f));  // Darker when clicked
            return;
        case EImGuiStyle::Danger:
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));    // Red
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));  // Red
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                                  ImVec4(0.9f, 0.3f, 0.3f, 1.0f));  // Lighter red on hover
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                                  ImVec4(0.7f, 0.1f, 0.1f, 1.0f));  // Darker red when clicked
            return;
    }

    ASSERT(false);
}

void ImGui_PopStyleColor() { ImGui::PopStyleColor(4); }

bool ImGui_WarningButton(const char* label, const ImVec2& size) {
    ImGui_PushStyleColor(EImGuiStyle::Warning);
    bool pressed = ImGui::Button(label, size);
    ImGui_PopStyleColor();
    return pressed;
}

bool ImGui_DangerButton(const char* label, const ImVec2& size) {
    ImGui_PushStyleColor(EImGuiStyle::Danger);
    bool pressed = ImGui::Button(label, size);
    ImGui_PopStyleColor();

    return pressed;
}

void BuildImGui(Arena* arena) {
    auto scratch = GetScratchArena();

    ImGui::Text("Type: %s", ToString(arena->Type).Str());

    ImGui::Separator();

    // Memory usage
    float usage_percent =
        arena->Size > 0 ? (float)arena->Offset / (float)arena->Size * 100.0f : 0.0f;
    ImGui::Text("Memory: %s / %s (%.2f%%)",
                ToMemoryString(scratch, arena->Offset).Str(),
                ToMemoryString(scratch, arena->Size).Str(),
                usage_percent);
    ImGui::SameLine();
    ImGui::ProgressBar(usage_percent / 100.0f, ImVec2(0.0f, 0.0f));

    ImGui::Text("Available: %s bytes", ToMemoryString(scratch, arena->Size - arena->Offset).Str());

    ImGui::Separator();

    // Statistics
    ImGui::Text("Alloc Calls: %u", arena->Stats.AllocCalls);
    ImGui::Text("Free Calls: %u", arena->Stats.FreeCalls);

    // Extendable arena specific info
    if (arena->Type == EArenaType::Extendable) {
        ImGui::Separator();
        ImGui::Text("Total Size: %llu bytes", arena->ExtendableData.TotalSize);
        ImGui::Text("Max Link Offset: %llu bytes", arena->ExtendableData.MaxLinkOffset);

        int chain_count = 0;
        Arena* current = arena;
        while (current && current->ExtendableData.NextArena) {
            chain_count++;
            current = current->ExtendableData.NextArena;
        }
        if (chain_count > 0) {
            ImGui::Text("Chain Links: %d", chain_count);
        }
    }
}

namespace imgui_widgets_private {

template <u32 BLOCK_COUNT, u32 BLOCK_SIZE>
void BuildImGui_BlockArena(BlockArena<BLOCK_SIZE, BLOCK_COUNT>* block_arena) {
    auto scratch = GetScratchArena();
    if (ImGui::TreeNodeEx(block_arena->Name.Str(), ImGuiTreeNodeFlags_Framed)) {
        ImGui::Text("Block Size: %u bytes, Block Count: %u", BLOCK_SIZE, BLOCK_COUNT);
        ImGui::Text("Allocated Blocks: %d / %u", block_arena->Stats.AllocatedBlocks, (BLOCK_COUNT));
        ImGui::SameLine();
        float usage = (float)block_arena->Stats.AllocatedBlocks / (float)(BLOCK_COUNT);
        ImGui::ProgressBar(usage, ImVec2(0.0f, 0.0f));
        ImGui::Separator();
        ImGui::Text("Total Memory: %s", ToMemoryString(scratch, block_arena->kTotalSize).Str());
        ImGui::Text(
            "Memory Used: %s",
            ToMemoryString(scratch, (u64)block_arena->Stats.AllocatedBlocks * BLOCK_SIZE).Str());
        ImGui::Text("Total Alloc Calls: %lld", block_arena->Stats.TotalAllocCalls);

        if (ImGui::TreeNodeEx("Allocated Blocks", ImGuiTreeNodeFlags_Framed)) {
            if (ImGui::BeginTable(
                    "AllocatedBlocksTable",
                    3,
                    ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                ImGui::TableSetupColumn("Block", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                ImGui::TableSetupColumn("Location", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Context", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                for (u32 i = 0; i < block_arena->kBlockCount; i++) {
                    if (!block_arena->BlockIndexAllocated(i)) {
                        continue;
                    }

                    auto* metadata = block_arena->GetBlockMetadataByIndex(i);

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%d", i);

                    ImGui::TableSetColumnIndex(1);
                    if (block_arena->BlocksMetadata[i].SourceLocation.file_name()) {
                        String path =
                            paths::CleanPathFromBazel(String(metadata->SourceLocation.file_name()));
                        ImGui::Text("%s:%d", path.Str(), metadata->SourceLocation.line());
                    } else {
                        ImGui::Text("<unknown>");
                    }

                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%s", metadata->ContextMsg.Str());
                }

                ImGui::EndTable();
            }

            ImGui::TreePop();
        }

        ImGui::TreePop();
    }
}

}  // namespace imgui_widgets_private

void BuildImGui(BlockArenaManager* bam) {
    using namespace imgui_widgets_private;

#define X(SIZE_NAME, BLOCK_SIZE, BLOCK_COUNT, BLOCK_SHIFT, ...)      \
    if (bam->_BlockArena_##SIZE_NAME) {                              \
        auto* block_arena = bam->_BlockArena_##SIZE_NAME;            \
        BuildImGui_BlockArena<BLOCK_COUNT, BLOCK_SIZE>(block_arena); \
    }  // namespace kdk
    BLOCK_ARENA_TYPES(X)
#undef X

#if 0

#endif
}

}  // namespace kdk

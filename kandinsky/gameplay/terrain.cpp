#include <kandinsky/gameplay/terrain.h>

#include <kandinsky/graphics/shader.h>
#include <kandinsky/imgui.h>
#include <kandinsky/platform.h>

namespace kdk {

namespace terrain_private {

Color32 TileToColor(ETerrainTileType tile) {
    switch (tile) {
        case ETerrainTileType::None: return Color32::FromRGBA(10, 10, 10, 255);
        case ETerrainTileType::Grass: return Color32::LimeGreen;
    }

    ASSERT(false);
    return Color32::NeonPink;
}

}  // namespace terrain_private

void Render(PlatformState* ps, const Terrain& terrain) {
    using namespace terrain_private;

    RenderState* rs = &ps->RenderState;

    Shader* normal_shader = FindShaderAsset(&ps->Assets, ps->Assets.BaseAssets.NormalShaderHandle);
    ASSERT(normal_shader);

    for (i32 z = 0; z < Terrain::kTileCount; z++) {
        for (i32 x = 0; x < Terrain::kTileCount; x++) {
            ETerrainTileType tile = GetTile(terrain, x, z);
            if (tile == ETerrainTileType::None) {
                continue;
            }

            Mat4 mmodel(1.0f);
            mmodel = Translate(mmodel, Vec3(x, -0.5f, z));
            // mmodel = Translate(mmodel, Vec3(x, 0, z));
            mmodel = Scale(mmodel, Vec3(0.9f));

            SetVec3(*normal_shader, "uColor", ToVec3(TileToColor(tile)));
            ChangeModelMatrix(rs, mmodel);
            Draw(&ps->Assets,
                 ps->Assets.BaseAssets.CubeModelHandle,
                 ps->Assets.BaseAssets.NormalShaderHandle,
                 ps->RenderState);
        }
    }
}

void BuildImGui(Terrain* terrain) {
    using namespace terrain_private;

    if (ImGui::TreeNodeEx("Tile Grid", ImGuiTreeNodeFlags_Framed)) {
        const float square_size = 20.0f;  // Size of each grid square in pixels
        const float grid_spacing = 2.0f;  // Spacing between squares

        ImVec2 cursor = ImGui::GetCursorScreenPos();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Draw the grid
        for (u32 z = 0; z < Terrain::kTileCount; z++) {
            for (u32 x = 0; x < Terrain::kTileCount; x++) {
                ETerrainTileType tile = GetTile(*terrain, x, z);

                ImVec2 square_min = ImVec2(cursor.x + x * (square_size + grid_spacing),
                                           cursor.y + z * (square_size + grid_spacing));
                ImVec2 square_max = ImVec2(square_min.x + square_size, square_min.y + square_size);

                // Choose color based on tile type
                ImU32 color = ToImU32(TileToColor(tile));
                const char* tooltip = "";
                switch (tile) {
                    case ETerrainTileType::None: break;
                    case ETerrainTileType::Grass: tooltip = "Grass"; break;
                }

                // Draw the square
                draw_list->AddRectFilled(square_min, square_max, color);

                // Add tooltip
                if (ImGui::IsMouseHoveringRect(square_min, square_max)) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Tile (%d, %d): %s", x, z, tooltip);
                    ImGui::EndTooltip();

                    // Detect left click on this tile
                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        // Handle the click here
                        printf("Clicked tile (%d, %d)\n", x, z);
                        if (tile == ETerrainTileType::None) {
                            SetTile(terrain, ETerrainTileType::Grass, x, z);
                        }
                    }
                }
            }
        }

        // Add enough vertical space for the grid
        ImGui::Dummy(ImVec2(Terrain::kTileCount * (square_size + grid_spacing),
                            Terrain::kTileCount * (square_size + grid_spacing)));

        ImGui::TreePop();
    }
}

}  // namespace kdk

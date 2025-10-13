#include <kandinsky/gameplay/terrain.h>

#include <kandinsky/debug.h>
#include <kandinsky/graphics/shader.h>
#include <kandinsky/imgui.h>
#include <kandinsky/platform.h>
#include <limits>

namespace kdk {

namespace terrain_private {

bool CheckBounds(i32 x, i32 z) {
    if (x < 0 || x >= Terrain::kTileCount) {
        return false;
    }

    if (z < 0 || z >= Terrain::kTileCount) {
        return false;
    }

    return true;
}

Color32 TileToColor(ETerrainTileType tile) {
    switch (tile) {
        case ETerrainTileType::None: return Color32::FromRGBA(10, 10, 10, 255);
        case ETerrainTileType::Grass: return Color32::LimeGreen;
        case ETerrainTileType::Path: return Color32::NewTan;
        case ETerrainTileType::COUNT: return Color32::NeonPink;
    }

    ASSERT(false);
    return Color32::NeonPink;
}

}  // namespace terrain_private

String ToString(ETerrainTileType type) {
    switch (type) {
        case ETerrainTileType::None: return "None"sv;
        case ETerrainTileType::Grass: return "Grass"sv;
        case ETerrainTileType::Path: return "Path"sv;
        case ETerrainTileType::COUNT: return "<count>"sv;
    }

    ASSERT(false);
    return "<unkninvalidown>"sv;
}

ETerrainTileType GetTileSafe(const Terrain& terrain, i32 x, i32 z) {
    if (!terrain_private::CheckBounds(x, z)) {
        return ETerrainTileType::None;
    }

    return GetTile(terrain, x, z);
}

i32 GetTileHeightSafe(const Terrain& terrain, i32 x, i32 z) {
    if (!terrain_private::CheckBounds(x, z)) {
        return NONE;
    }

    if (ETerrainTileType tile = GetTile(terrain, x, z); tile == ETerrainTileType::Grass) {
        return 1;
    }

    return NONE;
}

void Serialize(SerdeArchive* sa, Terrain* terrain) {
    // We serialize it into one string.

    if (sa->Mode == ESerdeMode::Serialize) {
        auto data = ArenaPushArray<char>(sa->TempArena, SQUARE(Terrain::kTileCount) + 1);
        for (i32 i = 0; i < terrain->Tiles.Size; i++) {
            char c = (char)terrain->Tiles[i];
            if (c == 0) {
                c = std::numeric_limits<char>::max();
            }
            data[i] = c;
        }
        data[data.size() - 1] = 0;

        String str(data.data(), data.size());
        Serde(sa, "Tiles", &str);
    } else {
        String str;
        Serde(sa, "Tiles", &str);
        ASSERT(str.Size == SQUARE(Terrain::kTileCount));

        for (i32 i = 0; i < terrain->Tiles.Size; i++) {
            char c = str[i];
            ETerrainTileType tile = ETerrainTileType::None;
            if (c != std::numeric_limits<char>::max()) {
                tile = (ETerrainTileType)c;
            }
            terrain->Tiles[i] = tile;
        }
    }
}

void Render(PlatformState* ps, const Terrain& terrain) {
    using namespace terrain_private;

    RenderState* rs = &ps->RenderState;

    Shader* normal_shader = FindShaderAsset(&ps->Assets, ps->Assets.BaseAssets.NormalShaderHandle);
    ASSERT(normal_shader);

    Vec2 terrain_size = Vec2(Terrain::kTileCount, Terrain::kTileCount);
    Debug::DrawBox(ps,
                   Vec3(terrain_size.x / 2.0f - 0.5f, 0, terrain_size.y / 2.0f - 0.5f),
                   Vec3(terrain_size.x / 2.0f, 0.0f, terrain_size.y / 2.0f),
                   Color32::Red,
                   5);

    SetEntity(rs, {});

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

        String ToString(ETerrainTileType type);
        // Draw the grid
        for (u32 z = 0; z < Terrain::kTileCount; z++) {
            for (u32 x = 0; x < Terrain::kTileCount; x++) {
                ETerrainTileType tile = GetTile(*terrain, x, z);

                ImVec2 square_min = ImVec2(cursor.x + x * (square_size + grid_spacing),
                                           cursor.y + z * (square_size + grid_spacing));
                ImVec2 square_max = ImVec2(square_min.x + square_size, square_min.y + square_size);

                // Choose color based on tile type
                ImU32 color = ToImU32(TileToColor(tile));
                String tooltip = ToString(tile);

                // Draw the square
                draw_list->AddRectFilled(square_min, square_max, color);

                // Add tooltip
                if (ImGui::IsMouseHoveringRect(square_min, square_max)) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Tile (%d, %d): %s", x, z, tooltip.Str());
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

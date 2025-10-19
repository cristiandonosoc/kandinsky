#include <kandinsky/gameplay/terrain.h>

#include <kandinsky/debug.h>
#include <kandinsky/graphics/shader.h>
#include <kandinsky/imgui.h>
#include <kandinsky/platform.h>
#include <bitset>
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

inline bool CheckBounds(const IVec2& grid_coord) { return CheckBounds(grid_coord.x, grid_coord.y); }

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

void InitTerrain(Terrain* terrain) {
    for (auto& from : terrain->FlowField) {
        from = IVec2(NONE, NONE);
    }
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

    if (ETerrainTileType tile = GetTile(terrain, x, z); tile != ETerrainTileType::None) {
        return 0;
    }

    return NONE;
}

Optional<IVec2> GetFlowTileSafe(const Terrain& terrain, i32 x, i32 z) {
    if (!terrain_private::CheckBounds(x, z)) {
        return IVec2(NONE, NONE);
    }

    IVec2 target = terrain.FlowField[z * Terrain::kTileCount + x];
    if (target.x == NONE || target.y == NONE) {
        return {};
    }

    return target;
}

void PlaceEntity(Terrain* terrain, EntityID entity_id, const IVec2& grid_coord) {
    if (!terrain_private::CheckBounds(grid_coord)) {
        return;
    }

    EntityID before = GetPlacedEntity(*terrain, grid_coord);
    ASSERT(IsNone(before));

    terrain->PlacedEntities[grid_coord.y * Terrain::kTileCount + grid_coord.x] = entity_id;
}

EntityID GetPlacedEntitySafe(const Terrain& terrain, const IVec2& grid_coord) {
    if (terrain_private::CheckBounds(grid_coord)) {
        return GetPlacedEntity(terrain, grid_coord);
    }

    return {};
}

void CalculateFlowField(PlatformState* ps, Terrain* terrain, const IVec2& target_pos) {
    ScopedArena scoped_arena = GetScopedArena(&ps->Memory.FrameArena);

    auto* queue = ArenaPushInit<Queue<IVec2, SQUARE(Terrain::kTileCount)>>(scoped_arena);
    auto* bitset = ArenaPushInit<std::bitset<SQUARE(Terrain::kTileCount)>>(scoped_arena);

    queue->Push(target_pos);

    auto to_index = [](const IVec2& pos) {
        return pos.y * Terrain::kTileCount + pos.x;
    };

    for (auto& from : terrain->FlowField) {
        from = IVec2(NONE, NONE);
    }

    while (!queue->IsEmpty()) {
        IVec2 pos = queue->Pop();

        // Get the neighbours.
        Array<IVec2, 4> neighbors = {
            IVec2{pos + IVec2(-1, 0)},
            IVec2{pos + IVec2(1, 0)},
            IVec2{pos + IVec2(0, -1)},
            IVec2{pos + IVec2(0, 1)},
        };

        for (const IVec2& neighbor_pos : neighbors) {
            // clang-format off
			if (neighbor_pos.x < 0 || neighbor_pos.x >= Terrain::kTileCount ||
				neighbor_pos.y < 0 || neighbor_pos.y >= Terrain::kTileCount) {
				continue;
			}
            // clang-format on

            i32 index = to_index(neighbor_pos);
            if (bitset->test(index)) {
                continue;
            }

            ETerrainTileType neighbor_tile = GetTile(*terrain, neighbor_pos.x, neighbor_pos.y);
            if (neighbor_tile == ETerrainTileType::Path) {
                bitset->set(index);
                terrain->FlowField[index] = pos;
                queue->Push(neighbor_pos);
            }
        }
    }
}

void DebugDrawFlowField(PlatformState* ps, const Terrain& terrain, Color32 color, float thickness) {
    for (i32 z = 0; z < Terrain::kTileCount; z++) {
        for (i32 x = 0; x < Terrain::kTileCount; x++) {
            IVec2 from = IVec2(x, z);
            IVec2 to = terrain.FlowField[z * Terrain::kTileCount + x];
            if (to.x == NONE || to.y == NONE) {
                continue;
            }

            Vec3 fromf = Vec3(from.x, 0.1f, from.y);
            Vec3 tof = Vec3(to.x, 0.1f, to.y);

            Vec3 dir = Normalize(tof - fromf);
            Debug::DrawArrow(ps, fromf, fromf + dir * 0.9f, color, 0.25f, thickness);
        }
    }
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

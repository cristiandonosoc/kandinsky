#pragma once

#include <kandinsky/core/color.h>
#include <kandinsky/core/math.h>
#include <kandinsky/core/string.h>

namespace kdk {

struct PlatformState;
struct SerdeArchive;

enum class ETerrainTileType : u8 {
    None = 0,
    Grass,
    Path,
    COUNT,
};
String ToString(ETerrainTileType type);

struct Terrain {
    static constexpr i32 kTileCount = 32;

    Array<ETerrainTileType, SQUARE(kTileCount)> Tiles = {};

    // Indicates for each node the path from it.
    Array<IVec2, SQUARE(kTileCount)> FlowField = {};
};
void InitTerrain(Terrain* terrain);

inline ETerrainTileType GetTile(const Terrain& terrain, i32 x, i32 z) {
    return terrain.Tiles[z * Terrain::kTileCount + x];
}

ETerrainTileType GetTileSafe(const Terrain& terrain, i32 x, i32 z);
inline ETerrainTileType GetTileSafe(const Terrain& terrain, const IVec2& grid_coord) {
    return GetTileSafe(terrain, grid_coord.x, grid_coord.y);
}
i32 GetTileHeightSafe(const Terrain& terrain, i32 x, i32 z);

inline void SetTile(Terrain* terrain, ETerrainTileType tile, i32 x, i32 z) {
    terrain->Tiles[z * Terrain::kTileCount + x] = tile;
}

void CalculateFlowField(PlatformState* ps, Terrain* terrain, const IVec2& target_pos);
void DebugDrawFlowField(PlatformState* ps,
                        const Terrain& terrain,
                        Color32 color = Color32::Cyan,
                        float thickness = 2.0f);

void Serialize(SerdeArchive* sa, Terrain* terrain);
void Render(PlatformState* ps, const Terrain& terrain);
void BuildImGui(Terrain* terrain);

}  // namespace kdk

#pragma once

#include <kandinsky/core/memory.h>

namespace kdk {

enum class ETerrainTileType : u8 {
    None = 0,
    Grass,
    COUNT,
};
String ToString(ETerrainTileType type);

struct Terrain {
    static constexpr i32 kTileCount = 32;

    Arena Memory = {};
    Array<ETerrainTileType, SQUARE(kTileCount)> Tiles = {};
};

inline ETerrainTileType GetTile(const Terrain& terrain, i32 x, i32 z) {
    return terrain.Tiles[z * Terrain::kTileCount + x];
}

ETerrainTileType GetTileSafe(const Terrain& terrain, i32 x, i32 z);
i32 GetTileHeightSafe(const Terrain& terrain, i32 x, i32 z);

inline void SetTile(Terrain* terrain, ETerrainTileType tile, i32 x, i32 z) {
    terrain->Tiles[z * Terrain::kTileCount + x] = tile;
}

void Render(PlatformState* ps, const Terrain& terrain);
void BuildImGui(Terrain* terrain);

}  // namespace kdk

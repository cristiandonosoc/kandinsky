#pragma once

#include <kandinsky/core/memory.h>

namespace kdk {

enum class ETerrainTileType : u8 {
    None = 0,
    Grass,
};

struct Terrain {
    static constexpr i32 kTileCount = 32;

    Arena Memory = {};
    Array<ETerrainTileType, SQUARE(kTileCount)> Tiles = {};
};

inline ETerrainTileType GetTile(const Terrain& terrain, u32 x, u32 z) {
    return terrain.Tiles[z * Terrain::kTileCount + x];
}
inline void SetTile(Terrain* terrain, ETerrainTileType tile, u32 x, u32 z) {
    terrain->Tiles[z * Terrain::kTileCount + x] = tile;
}

void Render(PlatformState* ps, const Terrain& terrain);
void BuildImGui(Terrain* terrain);

}  // namespace kdk

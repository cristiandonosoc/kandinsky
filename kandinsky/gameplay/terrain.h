#pragma once

#include <kandinsky/core/string.h>

namespace kdk {

struct PlatformState;
struct SerdeArchive;

enum class ETerrainTileType : u8 {
    None = 0,
    Grass,
    COUNT,
};
String ToString(ETerrainTileType type);

struct Terrain {
    static constexpr i32 kTileCount = 32;

    Array<ETerrainTileType, SQUARE(kTileCount)> Tiles = {};
    Array<i32, SQUARE(kTileCount)> Tiles2 = {};
};

inline ETerrainTileType GetTile(const Terrain& terrain, i32 x, i32 z) {
    return terrain.Tiles[z * Terrain::kTileCount + x];
}

ETerrainTileType GetTileSafe(const Terrain& terrain, i32 x, i32 z);
i32 GetTileHeightSafe(const Terrain& terrain, i32 x, i32 z);

inline void SetTile(Terrain* terrain, ETerrainTileType tile, i32 x, i32 z) {
    terrain->Tiles[z * Terrain::kTileCount + x] = tile;
}

void Serialize(SerdeArchive* sa, Terrain* terrain);
void Render(PlatformState* ps, const Terrain& terrain);
void BuildImGui(Terrain* terrain);

}  // namespace kdk

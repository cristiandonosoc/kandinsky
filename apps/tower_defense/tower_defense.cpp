#include <tower_defense/tower_defense.h>

#include <kandinsky/serde.h>

#include <SDL3/SDL.h>

#include <b64.h>

#include <optional>
#include <unordered_map>

namespace kdk {

void Serialize(SerdeArchive* sa, TileChunk& tc) {
    if (sa->Mode == ESerdeMode::Serialize) {
        u8 buf[1024];
        static_assert(sizeof(buf) > kTileChunkTotalSize + 1);
        for (u32 i = 0; i < kTileChunkTotalSize; i++) {
            buf[i] = (u8)tc.Tiles[i];
        }

        char* e = b64_encode(buf, kTileChunkTotalSize);
        DEFER { free(e); };

        String encoded_str = String(e, kTileChunkTotalSize);
        Serde(sa, "TileChunk", encoded_str);
    } else {
        String decoded_str;
        Serde(sa, "TileChunk", decoded_str);

        u8* d = b64_decode(decoded_str.Str(), decoded_str.Size);
        DEFER { free(d); };

        for (u32 i = 0; i < kTileChunkTotalSize; i++) {
            tc.Tiles[i] = (ETileType)d[i];
        }
    }
}

void Serialize(SerdeArchive* sa, TowerDefense& td) {
    SERDE(sa, td, TileChunk);
    SERDE(sa, td, EntityManager);
}

void EncodeDecode(const TileChunk& tc) {
    u8 buf[1024];
    for (u32 i = 0; i < tc.Tiles.size(); i++) {
        buf[i] = (u8)tc.Tiles[i];
    }

    char* encoded = b64_encode(buf, tc.Tiles.size());
    DEFER { free(encoded); };
    SDL_Log("Encoded: %s\n", encoded);
    unsigned char* decoded = b64_decode(encoded, strlen(encoded));
    DEFER { free(decoded); };

    for (u32 i = 0; i < tc.Tiles.size(); i++) {
        ASSERT(buf[i] == decoded[i]);
    }
}

// Validation --------------------------------------------------------------------------------------

namespace tower_defense_private {

std::optional<ValidationError> ValidateGridCoord(
    Arena* arena,
    Arena* scratch_arena,
    const TowerDefense& td,
    std::unordered_map<u32, EditorID>& registered_entities,
    const Entity& entity,
    const UVec2& grid_coord) {
    u32 coord = GridCoordID(grid_coord);
    auto found = registered_entities.find(coord);
    if (found != registered_entities.end()) {
        ValidationError error = {};
        error.entity_id = entity.EditorID;
        error.Message = Printf(arena,
                               "Coord %s already has an entity: %s",
                               ToString(scratch_arena, grid_coord).Str(),
                               ToString(scratch_arena, found->second).Str());
        return error;
    }
    registered_entities[coord] = entity.EditorID;

    if (coord >= kTileChunkTotalSize) {
        ValidationError error = {};
        error.entity_id = entity.EditorID;
        error.Message =
            Printf(arena, "Coord %s is out of bounds", ToString(scratch_arena, grid_coord).Str());
        return error;
    }

    if (EditorID in_grid_id = td.TileChunk.Entities[coord]; in_grid_id != entity.EditorID) {
        ValidationError error = {};
        error.entity_id = entity.EditorID;
        error.Message =
            Printf(arena,
                   "Saved entity in tiles (%llu) is not the same as this entityh (%llu)",
                   in_grid_id.Value,
                   entity.EditorID.Value);
        return error;
    }

    return {};
}

}  // namespace tower_defense_private

void Validate(Arena* arena, const TowerDefense& td, DynArray<ValidationError>* out) {
    using namespace tower_defense_private;

    auto scratch = GetScratchArena(arena);

    // TODO(cdc): Create arena friendly hash_map, set.
    std::unordered_map<u32, EditorID> registered_entities;

    for (auto it = GetIteratorT<Tower>(&td.EntityManager); it; it++) {
        if (auto error = ValidateGridCoord(arena,
                                           scratch.Arena,
                                           td,
                                           registered_entities,
                                           it->Entity,
                                           it->GridCoord);
            error.has_value()) {
            out->Push(arena, *error);
        }
    }

    for (auto it = GetIteratorT<Spawner>(&td.EntityManager); it; it++) {
        if (auto error = ValidateGridCoord(arena,
                                           scratch.Arena,
                                           td,
                                           registered_entities,
                                           it->Entity,
                                           it->GridCoord);
            error.has_value()) {
            out->Push(arena, *error);
        }
    }
}

}  // namespace kdk

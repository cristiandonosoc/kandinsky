#include <tower_defense/tower_defense.h>

#include <kandinsky/serde.h>

#include <SDL3/SDL.h>

#include <b64.h>

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

void Serialize(SerdeArchive* sa, TowerDefense& td) { SERDE(sa, td, TileChunk); }

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

}  // namespace kdk

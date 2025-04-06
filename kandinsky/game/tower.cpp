#include <kandinsky/game/tower.h>

#include <kandinsky/serde.h>

namespace kdk {

void Serialize(SerdeArchive* sa, Tower& tower) {
    SERDE(sa, tower, Entity);
    SERDE(sa, tower, GridCoord);
    SERDE(sa, tower, Color);
}

void Serialize(SerdeArchive* sa, Base& base) {
    SERDE(sa, base, Entity);
    SERDE(sa, base, GridCoord);
}

}  // namespace kdk

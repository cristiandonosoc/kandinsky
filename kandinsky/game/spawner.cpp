#include <kandinsky/game/spawner.h>

#include <kandinsky/serde.h>

namespace kdk {

void Serialize(SerdeArchive* sa, Spawner& spawner) {
    SERDE(sa, spawner, Entity);
    SERDE(sa, spawner, GridCoord);
}

void Serialize(SerdeArchive*, Enemy&) {
    ASSERTF(false, "We should not be serializing enemies, they are a runtime concept for now");
}

}  // namespace kdk

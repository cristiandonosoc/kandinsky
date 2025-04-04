#include <kandinsky/game/tower.h>

#include <kandinsky/serde.h>

namespace kdk {

void Serialize(SerdeArchive* sa, Tower& tower) { SERDE(sa, tower, Color); }

}  // namespace kdk

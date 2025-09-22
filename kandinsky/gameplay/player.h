#pragma once

#include <kandinsky/entity.h>

namespace kdk {

struct PlayerEntity {
	GENERATE_ENTITY(Player);
};

void Update(Entity* entity, PlayerEntity* player, float dt);
inline void Serialize(SerdeArchive*, PlayerEntity*) {}

} // namespace kdk

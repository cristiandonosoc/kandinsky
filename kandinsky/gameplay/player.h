#pragma once

#include <kandinsky/entity.h>

namespace kdk {

struct PlayerEntity {
	GENERATE_ENTITY(Player);
};
inline void Validate(const Scene*, const PlayerEntity&, FixedVector<ValidationError, 64>*) {}

void Update(Entity* entity, PlayerEntity* player, float dt);
inline void Serialize(SerdeArchive*, PlayerEntity*) {}

} // namespace kdk

#pragma once

#include <kandinsky/game/entity.h>

namespace kdk  {

struct Spawner {
	GENERATE_ENTITY(Spawner);

	UVec2 GridCoord = {};
};

void Serialize(SerdeArchive *sa, Spawner& spawner);

struct Enemy {
	GENERATE_ENTITY(Enemy);
};
void Serialize(SerdeArchive *sa, Enemy& enemy);

} // namespace kdk

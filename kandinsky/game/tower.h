#pragma once

#include <kandinsky/color.h>
#include <kandinsky/game/entity.h>

namespace kdk {

struct SerdeArchive;

struct Tower {
    GENERATE_ENTITY(Tower);

	UVec2 GridCoord = {};
    Color32 Color = Color32::White;
};

void Serialize(SerdeArchive* sa, Tower& tower);

struct Base {
	GENERATE_ENTITY(Base);

	UVec2 GridCoord = {};

};

void Serialize(SerdeArchive* sa, Base& base);

}  // namespace kdk

#pragma once

#include <kandinsky/color.h>
#include <kandinsky/game/entity.h>

namespace kdk {

struct SerdeArchive;

struct Tower {
    GENERATE_ENTITY(Tower);

    Color32 Color = Color32::White;
};

void Serialize(SerdeArchive* sa, Tower& tower);

}  // namespace kdk

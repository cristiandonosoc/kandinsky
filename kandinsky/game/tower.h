#pragma once

#include <kandinsky/color.h>
#include <kandinsky/game/entity.h>

namespace kdk {

struct Tower {
    GENERATE_ENTITY(Tower);

    Color32 Color = Color32::White;
};

}  // namespace kdk

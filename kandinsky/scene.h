#pragma once

#include <kandinsky/core/string.h>
#include <kandinsky/entity.h>

namespace kdk {

struct SerdeArchive;

struct Scene {
    FixedString<128> Name = {};
    FixedString<512> Path = {};
    EntityManager EntityManager = {};
};

void Serialize(SerdeArchive* sa, Scene* scene);

}  // namespace kdk

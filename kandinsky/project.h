#pragma once

#include <kandinsky/core/string.h>
#include <kandinsky/entity.h>
#include <kandinsky/core/serde.h>

namespace kdk {

struct ProjectHeader {
    FixedString<128> Name = {};
    FixedString<256> Description = {};
};

void Serialize(SerdeArchive* sa, ProjectHeader& header);

struct Project {
    ProjectHeader Header = {};
    EntityManager Entities = {};
};

void Serialize(SerdeArchive* sa, Project& project);


}  // namespace kdk

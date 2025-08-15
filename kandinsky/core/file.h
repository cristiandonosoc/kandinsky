#pragma once

#include <kandinsky/core/string.h>

#include <span>

namespace kdk {

struct Arena;

bool SaveFile(String path, std::span<u8> data);
std::span<u8> LoadFile(Arena* arena, String path);

}  // namespace kdk

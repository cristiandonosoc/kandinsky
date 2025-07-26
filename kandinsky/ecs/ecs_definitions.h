#pragma once

#include <kandinsky/defines.h>

namespace kdk {

enum class EECSComponentType : u8;  // Forward declare.

using ECSEntity = i32;  // 8-bit generation, 24-bit index.
inline i32 GetEntityIndex(ECSEntity entity) { return entity & 0xFFFFFF; }
inline u8 GetEntityGeneration(ECSEntity entity) { return (u8)(entity >> 24); }
inline ECSEntity BuildEntity(i32 index, u8 generation) {
    return generation << 24 | (index & 0xFFFFFF);
}

using ECSComponentIndex = i32;
using ECSEntitySignature = i32;

static constexpr i32 kMaxEntities = 4096;
static constexpr i32 kMaxComponentTypes = 31;
static constexpr i32 kNewEntitySignature = 1 << kMaxComponentTypes;  // Just the first bit set.

}  // namespace kdk

#pragma once

#include <kandinsky/defines.h>

#include <array>

namespace kdk {

using ECSEntity = i32;
using ECSEntitySignature = i32;

constexpr i32 kMaxEntities = 4096;

struct ECSEntityManager {
    std::array<ECSEntitySignature, kMaxEntities> Signatures = {};
    u32 EntityCount = 0;
    ECSEntity NextEntity = 0;
};

void Init(ECSEntityManager* eem);
void Shutdown(ECSEntityManager* eem);

ECSEntity CreateEntity(ECSEntityManager* eem, ECSEntity entity);
void DestroyEntity(ECSEntityManager* eem);

enum class EComponents : u8 {
	Transform,
};

}  // namespace kdk

#include <kandinsky/ecs/ecs_entity.h>

#include <kandinsky/defines.h>

namespace kdk {

void Init(ECSEntityManager* eem) {
    eem->EntityCount = 0;

    // Empty entities point to the *next* empty entity.
    // The last entity points to NONE.
    for (u32 i = 0; i < kMaxEntities; ++i) {
        eem->Signatures[i] = i + 1;
    }
    eem->Signatures.back() = NONE;
}

void Shutdown(ECSEntityManager* eem) {
    for (u32 i = 0; i < kMaxEntities; ++i) {
        eem->Signatures[i] = NONE;
    }
}

ECSEntity CreateEntity(ECSEntityManager* eem) {
    ASSERT(eem->EntityCount < kMaxEntities);

    // Find the next empty entity.
    ECSEntity entity = eem->NextEntity;
    ASSERT(entity != NONE);

    // Update the next entity pointer.
    eem->NextEntity = eem->Signatures[entity];
    eem->Signatures[entity] = 0;

    // Increment the entity count.
    eem->EntityCount++;
    return entity;
}

void DestroyEntity(ECSEntityManager* eem, ECSEntity entity) {
    ASSERT(entity != NONE);
    ASSERT(entity >= 0 && entity < kMaxEntities);
    ASSERT(eem->Signatures[entity] != NONE);

    // Mark the destroyed entity as the next (so we will fill that slot first).
    // We also mark that slot pointing to the prev next entity.
    eem->Signatures[entity] = eem->NextEntity;
    eem->NextEntity = entity;
	eem->EntityCount--;
}

}  // namespace kdk

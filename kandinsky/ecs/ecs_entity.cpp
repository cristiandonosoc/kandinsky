#include <kandinsky/ecs/ecs_entity.h>

#include <kandinsky/defines.h>

namespace kdk::ecs_entity_private {

ECSEntity BuildEntity(i32 index, u8 generation) { return generation << 24 | (index & 0xFFFFFF); }


}  // namespace kdk::ecs_entity_private

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
    i32 new_entity_index = eem->NextIndex;
    ASSERT(new_entity_index != NONE);

    // Negative signatures means that the entity is alive.
    ECSEntitySignature& new_entity_signature = eem->Signatures[new_entity_index];
    ASSERT(new_entity_signature >= 0);

    // Update the next entity pointer.
    eem->NextIndex = new_entity_signature;
    new_entity_signature = kNewEntitySignature;

    auto& new_entity_generation = eem->Generations[new_entity_index];
    new_entity_generation++;

    // Increment the entity count.
    eem->EntityCount++;
    return ecs_entity_private::BuildEntity(new_entity_index, new_entity_generation);
}

void DestroyEntity(ECSEntityManager* eem, ECSEntity entity) {
    ASSERT(entity != NONE);

    i32 index = GetEntityIndex(entity);
    ASSERT(index >= 0 && index < kMaxEntities);

    // Positive signatures means that the entity is not alive (and this slot is pointing to a empty
    // slot).
    ECSEntitySignature signature = eem->Signatures[index];
    if (signature >= 0) {
        return;
    }
    ASSERT(eem->Signatures[index] != NONE);

    // Since this is a live entity, we compare generations.
    u8 generation = GetEntityGeneration(entity);
    if (generation != eem->Generations[index]) {
        return;
    }

    // Mark the destroyed entity as the next (so we will fill that slot first).
    // We also mark that slot pointing to the prev next entity.
    eem->Signatures[index] = eem->NextIndex;
    eem->NextIndex = index;

    eem->EntityCount--;
}

bool IsValid(const ECSEntityManager& eem, ECSEntity entity) {
    if (entity == NONE) {
        return false;
    }

    i32 index = GetEntityIndex(entity);
    ASSERT(index > 0 && index < kMaxEntities);

    // Live entities have a negative signature.
    ECSEntitySignature signature = eem.Signatures[index];
    if (signature >= 0) {
        return false;
    }

    // We simply compare generations.
    u8 generation = GetEntityGeneration(entity);
    if (eem.Generations[index] != generation) {
        return false;
    }

    return true;
}

}  // namespace kdk

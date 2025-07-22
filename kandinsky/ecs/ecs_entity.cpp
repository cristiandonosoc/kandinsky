#include <kandinsky/ecs/ecs_entity.h>

#include <kandinsky/assert.h>



namespace kdk {

void Init(ECSEntityManager* eem) {
    eem->EntityCount = 0;

    // Empty entities point to the *next* empty entity.
    for (u32 i = 0; i < kMaxEntities; ++i) {
        eem->Signatures[i] = i + 1;
    }

    // The last entity points to NONE.
    eem->Signatures[kMaxEntities - 1] = NONE;
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
	if (entity == NONE) {
		return NONE;  // No more entities available.
	}

	// Update the next entity pointer.
	eem->NextEntity = eem->Signatures[entity];

	// Mark this entity as used by setting its signature to NONE.
	eem->Signatures[entity] = NONE;

	// Increment the entity count.
	eem->EntityCount++;
	return entity;
}

void DestroyEntity(ECSEntityManager* eem, ECSEntity entity) {
	ASSERT(entity != NONE);
	ASSERT(entity > 0 && entity < kMaxEntities);
	ASSERT(eem->Signatures[entity] != NONE);

	ECSEntity prev_next_entity = eem->NextEntity;

	// Mark the destroyed entity as the next (so we will fill that slot first).
	// We also mark that slot pointing to the prev next entity.
	eem->NextEntity = entity;
	eem->Signatures[entity] = prev_next_entity;
}

}  // namespace kdk

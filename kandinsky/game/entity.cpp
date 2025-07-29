#include <kandinsky/game/entity.h>

#include <kandinsky/defines.h>
#include <kandinsky/intrin.h>

namespace kdk::entity_private {

void AddComponentToSignature(EntitySignature* signature, EEntityComponentType component_type) {
    ASSERT(component_type < EEntityComponentType::COUNT);
    *signature |= (1 << (u8)component_type);
}

void RemoveComponentFromSignature(EntitySignature* signature, EEntityComponentType component_type) {
    ASSERT(component_type < EEntityComponentType::COUNT);
    *signature &= ~(1 << (u8)component_type);
}

}  // namespace kdk::entity_private

namespace kdk {

bool Matches(const EntitySignature& signature, EEntityComponentType component_type) {
    u8 offset = 1 << (u8)component_type;
    ASSERT(offset < kMaxComponentTypes);
    return (signature & offset) != 0;
}

void Init(EntityManager* eem) {
    eem->EntityCount = 0;

    // Empty entities point to the *next* empty entity.
    // The last entity points to NONE.
    for (u32 i = 0; i < kMaxEntities; ++i) {
        eem->Signatures[i] = i + 1;
    }
    eem->Signatures.back() = NONE;

    // Init the component holders.
#define X(component_enum_name, ...)                       \
    case EEntityComponentType::component_enum_name:          \
        eem->component_enum_name##ComponentHolder.Init(); \
        break;

    for (u8 i = 0; i < (u8)EEntityComponentType::COUNT; i++) {
        switch ((EEntityComponentType)i) {
            ECS_COMPONENT_TYPES(X)
            default: ASSERTF(false, "Unknown component type %u", i); break;
        }
    }
#undef X
}

void Shutdown(EntityManager* eem) {
    // Shutdown the component holders.
#define X(component_enum_name, ...)                           \
    case EEntityComponentType::component_enum_name:              \
        eem->component_enum_name##ComponentHolder.Shutdown(); \
        break;

    // In reverse order.
    for (i32 i = (i32)EEntityComponentType::COUNT - 1; i >= 0; i--) {
        switch ((EEntityComponentType)i) {
            ECS_COMPONENT_TYPES(X)
            default: ASSERTF(false, "Unknown component type %u", i); break;
        }
    }
#undef X

    for (u32 i = 0; i < kMaxEntities; ++i) {
        eem->Signatures[i] = NONE;
    }
}

Entity CreateEntity(EntityManager* eem, EntityData** out_data) {
    ASSERT(eem->EntityCount < kMaxEntities);

    // Find the next empty entity.
    i32 new_entity_index = eem->NextIndex;
    ASSERT(new_entity_index != NONE);

    // Negative signatures means that the entity is alive.
    EntitySignature& new_entity_signature = eem->Signatures[new_entity_index];
    ASSERT(new_entity_signature >= 0);

    // Update the next entity pointer.
    eem->NextIndex = new_entity_signature;
    new_entity_signature = kNewEntitySignature;

    auto& new_entity_generation = eem->Generations[new_entity_index];
    new_entity_generation++;

    // Reset the entity data.
    auto& entity_data = eem->EntityDatas[new_entity_index];
    entity_data = {};

    // Increment the entity count.
    eem->EntityCount++;
    Entity entity = BuildEntity(new_entity_index, new_entity_generation);

    if (out_data) {
        *out_data = &entity_data;
    }

    return entity;
}

void DestroyEntity(EntityManager* eem, Entity entity) {
    ASSERT(entity != NONE);

    i32 index = GetEntityIndex(entity);
    ASSERT(index >= 0 && index < kMaxEntities);

    // Positive signatures means that the entity is not alive (and this slot is pointing to a empty
    // slot).
    EntitySignature signature = eem->Signatures[index];
    if (!IsLive(signature)) {
        return;
    }
    ASSERT(eem->Signatures[index] != NONE);

    // Since this is a live entity, we compare generations.
    u8 generation = GetEntityGeneration(entity);
    if (generation != eem->Generations[index]) {
        return;
    }

    // Go over all components and remove them from the entity.
    i32 signature_bitfield = (i32)signature;
    while (signature_bitfield) {
        // Get the component type from the signature.
        EEntityComponentType component_type = (EEntityComponentType)BitScanForward(signature_bitfield);
        if (component_type >= EEntityComponentType::COUNT) {
            break;
        }
        // Remove the component from the entity.
        RemoveComponent(eem, entity, component_type);
        signature_bitfield &= signature_bitfield - 1;  // Clear the lowest bit.
    }

    // Mark the destroyed entity as the next (so we will fill that slot first).
    // We also mark that slot pointing to the prev next entity.
    eem->Signatures[index] = eem->NextIndex;
    eem->NextIndex = index;

    eem->EntityCount--;
}

bool IsValid(const EntityManager& eem, Entity entity) {
    if (entity == NONE) {
        return false;
    }

    i32 index = GetEntityIndex(entity);
    ASSERT(index >= 0 && index < kMaxEntities);

    // Live entities have a negative signature.
    EntitySignature signature = eem.Signatures[index];
    if (!IsLive(signature)) {
        return false;
    }

    // We simply compare generations.
    u8 generation = GetEntityGeneration(entity);
    if (eem.Generations[index] != generation) {
        return false;
    }

    return true;
}

EntitySignature* GetEntitySignature(EntityManager* eem, Entity entity) {
    if (!IsValid(*eem, entity)) {
        return nullptr;
    }

    i32 index = GetEntityIndex(entity);
    ASSERT(index >= 0 && index < kMaxEntities);
    return &eem->Signatures[index];
}

EntityData* GetEntityData(EntityManager* eem, Entity entity) {
    if (!IsValid(*eem, entity)) {
        return nullptr;
    }

    i32 index = GetEntityIndex(entity);
    ASSERT(index >= 0 && index < kMaxEntities);
    return &eem->EntityDatas[index];
}

bool AddComponent(EntityManager* eem, Entity entity, EEntityComponentType component_type) {
    auto* signature = GetEntitySignature(eem, entity);
    if (!signature) {
        return false;
    }

    // If it already has the component, we return false.
    if (Matches(*signature, component_type)) {
        return false;
    }

    // X-macro to find the component holder.
#define X(component_enum_name, ...)                                  \
    case EEntityComponentType::component_enum_name:                     \
        eem->component_enum_name##ComponentHolder.AddEntity(entity); \
        break;

    switch (component_type) {
        ECS_COMPONENT_TYPES(X)
        default: ASSERTF(false, "Unknown component type %d", (u8)component_type); return false;
    }
#undef X

    entity_private::AddComponentToSignature(signature, component_type);
    return true;
}

bool RemoveComponent(EntityManager* eem, Entity entity, EEntityComponentType component_type) {
    auto* signature = GetEntitySignature(eem, entity);
    if (!signature) {
        return false;
    }

    // If it doesn't have the component, we return false.
    if (!Matches(*signature, component_type)) {
        return false;
    }

    // X-macro to find the component holder.
#define X(component_enum_name, ...)                                     \
    case EEntityComponentType::component_enum_name:                        \
        eem->component_enum_name##ComponentHolder.RemoveEntity(entity); \
        break;

    switch (component_type) {
        ECS_COMPONENT_TYPES(X)
        default: ASSERTF(false, "Unknown component type %d", (u8)component_type); return false;
    }
#undef X

    entity_private::RemoveComponentFromSignature(signature, component_type);
    return true;
}

i32 GetComponentCount(const EntityManager& eem, EEntityComponentType component_type) {
    ASSERT(component_type < EEntityComponentType::COUNT);

    // X-macro to find the component holder.
#define X(component_enum_name, ...)                                     \
    case EEntityComponentType::component_enum_name:                        \
        return eem.component_enum_name##ComponentHolder.ComponentCount; \
        break;

    switch (component_type) {
        ECS_COMPONENT_TYPES(X)
        default: ASSERTF(false, "Unknown component type %d", (u8)component_type); return 0;
    }
#undef X
}

EntityComponentIndex GetComponentIndex(const EntityManager& eem,
                                    Entity entity,
                                    EEntityComponentType component_type) {
    ASSERT(component_type < EEntityComponentType::COUNT);
    if (!IsValid(eem, entity)) {
        return NONE;
    }

    i32 entity_index = GetEntityIndex(entity);
    if (!Matches(eem.Signatures[entity_index], component_type)) {
        return NONE;  // Entity does not have this component.
    }

    // X-macro to find the component holder.
#define X(component_enum_name, ...)                                        \
    case EEntityComponentType::component_enum_name: {                         \
        auto& component_holder = eem.component_enum_name##ComponentHolder; \
        return component_holder.EntityToComponent[entity_index];           \
        break;                                                             \
    }

    switch (component_type) {
        ECS_COMPONENT_TYPES(X)
        default: ASSERTF(false, "Unknown component type %d", (u8)component_type); return NONE;
    }
#undef X
}

Entity GetOwningEntity(const EntityManager& eem,
                          EEntityComponentType component_type,
                          EntityComponentIndex component_index) {
    ASSERT(component_type < EEntityComponentType::COUNT);

    // X-macro to find the component holder.
#define X(component_enum_name, ...)                                                        \
    case EEntityComponentType::component_enum_name: {                                         \
        auto& component_holder = eem.component_enum_name##ComponentHolder;                 \
        ASSERT(component_index >= 0 && component_index < component_holder.kMaxComponents); \
        return component_holder.ComponentToEntity[component_index];                        \
    }

    switch (component_type) {
        ECS_COMPONENT_TYPES(X)
        default: ASSERTF(false, "Unknown component type %d", (u8)component_type); return NONE;
    }
#undef X
}

}  // namespace kdk

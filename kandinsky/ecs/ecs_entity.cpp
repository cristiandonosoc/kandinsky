#include <kandinsky/ecs/ecs_entity.h>

#include <kandinsky/defines.h>
#include <kandinsky/intrin.h>

namespace kdk::ecs_entity_private {

void AddComponentToSignature(ECSEntitySignature* signature, EECSComponentType component_type) {
    ASSERT(component_type < EECSComponentType::COUNT);
    *signature |= (1 << (u8)component_type);
}

void RemoveComponentFromSignature(ECSEntitySignature* signature, EECSComponentType component_type) {
    ASSERT(component_type < EECSComponentType::COUNT);
    *signature &= ~(1 << (u8)component_type);
}

}  // namespace kdk::ecs_entity_private

namespace kdk {

const char* ToString(EECSComponentType component_type) {
    // X-macro to find the component holder.
#define X(component_enum_name, ...) \
    case EECSComponentType::component_enum_name: return #component_enum_name;

    switch (component_type) {
        ECS_COMPONENT_TYPES(X)
        default:
            ASSERTF(false, "Unknown component type %d", (u8)component_type);
            return "<invalid>";
    }
#undef X
}

bool Matches(const ECSEntitySignature& signature, EECSComponentType component_type) {
    u8 offset = 1 << (u8)component_type;
    ASSERT(offset < kMaxComponentTypes);
    return (signature & offset) != 0;
}

void Init(ECSEntityManager* eem) {
    eem->EntityCount = 0;

    // Empty entities point to the *next* empty entity.
    // The last entity points to NONE.
    for (u32 i = 0; i < kMaxEntities; ++i) {
        eem->Signatures[i] = i + 1;
    }
    eem->Signatures.back() = NONE;

    // Init the component holders.
#define X(component_enum_name, ...) \
    case EECSComponentType::component_enum_name: eem->component_enum_name##Holder.Init(); break;

    for (u8 i = 0; i < (u8)EECSComponentType::COUNT; i++) {
        switch ((EECSComponentType)i) {
            ECS_COMPONENT_TYPES(X)
            default: ASSERTF(false, "Unknown component type %u", i); break;
        }
    }
#undef X
}

void Shutdown(ECSEntityManager* eem) {
    // Shutdown the component holders.
#define X(component_enum_name, ...) \
    case EECSComponentType::component_enum_name: eem->component_enum_name##Holder.Shutdown(); break;

    // In reverse order.
    for (i32 i = (i32)EECSComponentType::COUNT - 1; i >= 0; i--) {
        switch ((EECSComponentType)i) {
            ECS_COMPONENT_TYPES(X)
            default: ASSERTF(false, "Unknown component type %u", i); break;
        }
    }
#undef X

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
    return BuildEntity(new_entity_index, new_entity_generation);
}

void DestroyEntity(ECSEntityManager* eem, ECSEntity entity) {
    ASSERT(entity != NONE);

    i32 index = GetEntityIndex(entity);
    ASSERT(index >= 0 && index < kMaxEntities);

    // Positive signatures means that the entity is not alive (and this slot is pointing to a empty
    // slot).
    ECSEntitySignature signature = eem->Signatures[index];
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
        EECSComponentType component_type = (EECSComponentType)BitScanForward(signature);
        if (component_type >= EECSComponentType::COUNT) {
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

bool IsValid(const ECSEntityManager& eem, ECSEntity entity) {
    if (entity == NONE) {
        return false;
    }

    i32 index = GetEntityIndex(entity);
    ASSERT(index >= 0 && index < kMaxEntities);

    // Live entities have a negative signature.
    ECSEntitySignature signature = eem.Signatures[index];
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

std::pair<bool, ECSEntitySignature*> GetEntitySignature(ECSEntityManager* eem, ECSEntity entity) {
    if (!IsValid(*eem, entity)) {
        return {false, nullptr};
    }

    return {true, &eem->Signatures[GetEntityIndex(entity)]};
}

bool AddComponent(ECSEntityManager* eem, ECSEntity entity, EECSComponentType component_type) {
    auto [ok, signature] = GetEntitySignature(eem, entity);
    if (!ok) {
        return false;
    }

    // If it already has the component, we return false.
    if (Matches(*signature, component_type)) {
        return false;
    }

    // X-macro to find the component holder.
#define X(component_enum_name, ...)                         \
    case EECSComponentType::component_enum_name:            \
        eem->component_enum_name##Holder.AddEntity(entity); \
        break;

    switch (component_type) {
        ECS_COMPONENT_TYPES(X)
        default: ASSERTF(false, "Unknown component type %d", (u8)component_type); return false;
    }
#undef X

    ecs_entity_private::AddComponentToSignature(signature, component_type);
    return true;
}

bool RemoveComponent(ECSEntityManager* eem, ECSEntity entity, EECSComponentType component_type) {
    auto [ok, signature] = GetEntitySignature(eem, entity);
    if (!ok) {
        return false;
    }

    // If it doesn't have the component, we return false.
    if (!Matches(*signature, component_type)) {
        return false;
    }

    // X-macro to find the component holder.
#define X(component_enum_name, ...)                            \
    case EECSComponentType::component_enum_name:               \
        eem->component_enum_name##Holder.RemoveEntity(entity); \
        break;

    switch (component_type) {
        ECS_COMPONENT_TYPES(X)
        default: ASSERTF(false, "Unknown component type %d", (u8)component_type); return false;
    }
#undef X

    ecs_entity_private::RemoveComponentFromSignature(signature, component_type);
    return true;
}

}  // namespace kdk

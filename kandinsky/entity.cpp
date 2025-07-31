#include <kandinsky/entity.h>

#include <kandinsky/defines.h>
#include <kandinsky/intrin.h>

#include <kandinsky/graphics/light.h>

#include <SDL3/SDL.h>
#include "kandinsky/math.h"

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

template <typename T, i32 SIZE>
struct EntityComponentHolder {
    static constexpr i32 kMaxComponents = SIZE;

    std::array<EntityComponentIndex, kMaxEntities> EntityToComponent;
    std::array<Entity, SIZE> ComponentToEntity;
    std::array<T, SIZE> Components = {};
    EntityManager* Owner = nullptr;
    EntityComponentIndex NextComponent = 0;
    i32 ComponentCount = 0;

    void Init(EntityManager* entity_manager);
    void Shutdown() {}

    std::pair<EntityComponentIndex, T*> AddEntity(Entity entity);
    std::pair<EntityComponentIndex, T*> GetEntity(Entity entity);
    void RemoveEntity(Entity entity);
};

struct EntityComponentSet {
    // Create the component arrays.
#define X(component_enum_name, component_struct_name, component_max_count, ...) \
    EntityComponentHolder<component_struct_name, component_max_count>           \
        component_enum_name##ComponentHolder;

    ECS_COMPONENT_TYPES(X)
#undef X
};

bool Matches(const EntitySignature& signature, EEntityComponentType component_type) {
    u8 offset = 1 << (u8)component_type;
    ASSERT(offset < kMaxComponentTypes);
    return (signature & offset) != 0;
}

const char* ToString(EEntityComponentType component_type) {
    // X-macro to find the component holder.
#define X(component_enum_name, ...) \
    case EEntityComponentType::component_enum_name: return #component_enum_name;

    switch (component_type) {
        ECS_COMPONENT_TYPES(X)
        default:
            ASSERTF(false, "Unknown component type %d", (u8)component_type);
            return "<invalid>";
    }
#undef X
}

void Init(Arena* arena, EntityManager* eem) {
    eem->EntityCount = 0;

    // Empty entities point to the *next* empty entity.
    // The last entity points to NONE.
    for (u32 i = 0; i < kMaxEntities; ++i) {
        eem->Signatures[i] = i + 1;
    }
    eem->Signatures.back() = NONE;

    eem->Components = ArenaPushInit<EntityComponentSet>(arena);

    // Init the component holders.
#define X(component_enum_name, ...)                                      \
    case EEntityComponentType::component_enum_name:                      \
        eem->Components->component_enum_name##ComponentHolder.Init(eem); \
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
#define X(component_enum_name, ...)                                       \
    case EEntityComponentType::component_enum_name:                       \
        eem->Components->component_enum_name##ComponentHolder.Shutdown(); \
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

    // Increment the entity count.
    eem->EntityCount++;
    Entity entity_id = BuildEntity(new_entity_index, new_entity_generation);

    // Reset the entity data.
    auto& entity_data = eem->EntityDatas[new_entity_index];
    entity_data = {
        .EntityID = entity_id,
    };

    if (out_data) {
        *out_data = &entity_data;
    }

    return entity_id;
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
        EEntityComponentType component_type =
            (EEntityComponentType)BitScanForward(signature_bitfield);
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
    return const_cast<EntityData*>(GetEntityData(*eem, entity));
}

const EntityData* GetEntityData(const EntityManager& eem, Entity entity) {
    if (!IsValid(eem, entity)) {
        return nullptr;
    }

    i32 index = GetEntityIndex(entity);
    ASSERT(index >= 0 && index < kMaxEntities);
    return &eem.EntityDatas[index];
}

void UpdateModelMatrices(EntityManager* eem) {
    // TODO(cdc): Would it be faster to just calculate them all always?
    //            This sounds parallelizable...
    i32 found_count = 0;
    for (i32 i = 0; i < kMaxEntities; i++) {
        if (IsLive(eem->Signatures[i])) {
            EntityData& entity = eem->EntityDatas[i];
            CalculateModelMatrix(entity.Transform, &entity.M_Model);
            found_count++;
        }

        if (found_count >= eem->EntityCount) {
            break;
        }
    }
}

EntityComponentIndex AddComponent(EntityManager* eem,
                                  Entity entity,
                                  EEntityComponentType component_type,
                                  void** out) {
    auto* signature = GetEntitySignature(eem, entity);
    if (!signature) {
        return NONE;
    }

    // If it already has the component, we return false.
    if (Matches(*signature, component_type)) {
        return NONE;
    }

    EntityComponentIndex out_index = NONE;

    // X-macro to find the component holder.
#define X(component_enum_name, ...)                                                  \
    case EEntityComponentType::component_enum_name: {                                \
        auto [index, component_ptr] =                                                \
            eem->Components->component_enum_name##ComponentHolder.AddEntity(entity); \
        if (out) {                                                                   \
            *out = component_ptr;                                                    \
        }                                                                            \
        out_index = index;                                                           \
        break;                                                                       \
    }

    switch (component_type) {
        ECS_COMPONENT_TYPES(X)
        default: ASSERTF(false, "Unknown component type %d", (u8)component_type); return false;
    }
#undef X

    entity_private::AddComponentToSignature(signature, component_type);
    return out_index;
}

EntityComponentIndex GetComponent(EntityManager* eem,
                                  Entity entity,
                                  EEntityComponentType component_type,
                                  void** out) {
    auto* signature = GetEntitySignature(eem, entity);
    if (!signature) {
        return NONE;
    }

    // If it already has the component, we return false.
    if (Matches(*signature, component_type)) {
        return NONE;
    }

    EntityComponentIndex out_index = NONE;

    // X-macro to find the component holder.
#define X(component_enum_name, ...)                                                  \
    case EEntityComponentType::component_enum_name: {                                \
        auto [component_index, component_ptr] =                                      \
            eem->Components->component_enum_name##ComponentHolder.GetEntity(entity); \
        if (out) {                                                                   \
            *out = component_ptr;                                                    \
        }                                                                            \
        out_index = component_index;                                                 \
        break;                                                                       \
    }

    switch (component_type) {
        ECS_COMPONENT_TYPES(X)
        default: ASSERTF(false, "Unknown component type %d", (u8)component_type); return false;
    }
#undef X

    entity_private::AddComponentToSignature(signature, component_type);
    return out_index;
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
#define X(component_enum_name, ...)                                                 \
    case EEntityComponentType::component_enum_name:                                 \
        eem->Components->component_enum_name##ComponentHolder.RemoveEntity(entity); \
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
#define X(component_enum_name, ...)                                                 \
    case EEntityComponentType::component_enum_name:                                 \
        return eem.Components->component_enum_name##ComponentHolder.ComponentCount; \
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
#define X(component_enum_name, ...)                                                    \
    case EEntityComponentType::component_enum_name: {                                  \
        auto& component_holder = eem.Components->component_enum_name##ComponentHolder; \
        return component_holder.EntityToComponent[entity_index];                       \
        break;                                                                         \
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
    case EEntityComponentType::component_enum_name: {                                      \
        auto& component_holder = eem.Components->component_enum_name##ComponentHolder;     \
        ASSERT(component_index >= 0 && component_index < component_holder.kMaxComponents); \
        return component_holder.ComponentToEntity[component_index];                        \
    }

    switch (component_type) {
        ECS_COMPONENT_TYPES(X)
        default: ASSERTF(false, "Unknown component type %d", (u8)component_type); return NONE;
    }
#undef X
}

// TEMPLATE IMPLEMENTATION -------------------------------------------------------------------------

template <typename T, i32 SIZE>
void EntityComponentHolder<T, SIZE>::Init(EntityManager* entity_manager) {
    EEntityComponentType component_type = T::kComponentType;

    for (i32 i = 0; i < SIZE; i++) {
        Components[i] = {};
    }

    for (auto& elem : EntityToComponent) {
        elem = NONE;
    }

    // Empty components point to the *next* empty component.
    // The last component points to NONE.
    for (i32 i = 0; i < SIZE; i++) {
        ComponentToEntity[i] = i + 1;
    }
    ComponentToEntity.back() = NONE;

    ASSERT(Owner == nullptr);
    Owner = entity_manager;

    SDL_Log("Initialized ComponentHolder: %s\n", ToString(component_type));
}

template <typename T, i32 SIZE>
std::pair<EntityComponentIndex, T*> EntityComponentHolder<T, SIZE>::AddEntity(Entity entity) {
    i32 entity_index = GetEntityIndex(entity);

    ASSERT(entity_index >= 0 && entity_index < kMaxEntities);
    ASSERT(ComponentCount < SIZE);
    ASSERT(EntityToComponent[entity_index] == NONE);

    // Find the next empty entity.
    EntityComponentIndex component_index = NextComponent;
    ASSERT(component_index != NONE);

    // Update the translation arrays.
    NextComponent = ComponentToEntity[component_index];
    ComponentToEntity[component_index] = entity;
    EntityToComponent[entity_index] = component_index;

    ComponentCount++;

    // Reset the component.
    T* component = &Components[component_index];
    *component = {
        ._EntityManager = Owner,
        ._OwnerID = entity,
        ._ComponentIndex = component_index,
    };

    return {component_index, component};
}

template <typename T, i32 SIZE>
std::pair<EntityComponentIndex, T*> EntityComponentHolder<T, SIZE>::GetEntity(Entity entity) {
    i32 entity_index = GetEntityIndex(entity);

    ASSERT(entity_index >= 0 && entity_index < kMaxEntities);
    ASSERT(ComponentCount > 0);
    EntityComponentIndex component_index = EntityToComponent[entity_index];
    ASSERT(component_index != NONE);

    T* component = &Components[component_index];
    ASSERT(component->_EntityManager == Owner);
    ASSERT(component->_OwnerID == entity);
    ASSERT(component->_ComponentIndex == component_index);
    return {component_index, component};
}

template <typename T, i32 SIZE>
void EntityComponentHolder<T, SIZE>::RemoveEntity(Entity entity) {
    i32 entity_index = GetEntityIndex(entity);

    ASSERT(entity_index >= 0 && entity_index < kMaxEntities);
    ASSERT(ComponentCount > 0);

    // Get the component index for this entity
    EntityComponentIndex component_index = EntityToComponent[entity_index];
    ASSERT(component_index != NONE);

    // Update the translation arrays.
    EntityToComponent[entity_index] = NONE;
    ComponentToEntity[component_index] = NextComponent;
    NextComponent = component_index;

    ComponentCount--;
}

}  // namespace kdk

#include <kandinsky/entity_manager.h>

#include <SDL3/SDL_Log.h>

namespace kdk {

namespace entity_manager_private {

void AddComponentToSignature(EntitySignature* signature, EEntityComponentType component_type) {
    ASSERT(component_type < EEntityComponentType::COUNT);
    *signature |= (1 << (u8)component_type);
}

void RemoveComponentFromSignature(EntitySignature* signature, EEntityComponentType component_type) {
    ASSERT(component_type < EEntityComponentType::COUNT);
    *signature &= ~(1 << (u8)component_type);
}

}  // namespace entity_manager_private

void Init(EntityManager* em, const InitEntityManagerOptions& options) {
    em->EntityCount = 0;
    em->NextIndex = 0;

    // Empty entities point to the *next* empty entity.
    // NOTE: The last entity points to an invalid slot.
    for (u32 i = 0; i < kMaxEntities; ++i) {
        em->Signatures[i] = i + 1;
    }

    // Init the component holders.
#define X(component_enum_name, ...)                        \
    case EEntityComponentType::component_enum_name:        \
        em->component_enum_name##ComponentHolder.Init(em); \
        break;

    for (u8 i = 0; i < (u8)EEntityComponentType::COUNT; i++) {
        switch ((EEntityComponentType)i) {
            COMPONENT_TYPES(X)
            default: ASSERTF(false, "Unknown component type %u", i); break;
        }
    }
#undef X

    if (options.Recalculate) {
        // Recalculate the next index chain.
        Recalculate(em);
    }
}

void Shutdown(EntityManager* em) {
    // Shutdown the component holders.
#define X(component_enum_name, ...)                          \
    case EEntityComponentType::component_enum_name:          \
        em->component_enum_name##ComponentHolder.Shutdown(); \
        break;

    // In reverse order.
    for (i32 i = (i32)EEntityComponentType::COUNT - 1; i >= 0; i--) {
        switch ((EEntityComponentType)i) {
            COMPONENT_TYPES(X)
            default: ASSERTF(false, "Unknown component type %u", i); break;
        }
    }
#undef X

    for (u32 i = 0; i < kMaxEntities; ++i) {
        em->Signatures[i] = NONE;
    }
}

void Recalculate(EntityManager* em) {
    em->NextIndex = 0;
    // We go from back to front adjusting the chain.
    for (i16 i = kMaxEntities - 1; i >= 0; i--) {
        // If the entity is alive, we skip it.
        if (IsLive(em->Signatures[i])) {
            continue;
        }

        // This entity is dead, we add it to the free list.
        em->Signatures[i] = em->NextIndex;
        em->NextIndex = i;
    }

    // Recalculate components.
#define X(component_enum_name, ...)                               \
    case EEntityComponentType::component_enum_name:               \
        em->component_enum_name##ComponentHolder.Recalculate(em); \
        break;

    for (u8 i = 0; i < (u8)EEntityComponentType::COUNT; i++) {
        switch ((EEntityComponentType)i) {
            COMPONENT_TYPES(X)
            default: ASSERTF(false, "Unknown component type %u", i); break;
        }
    }
#undef X
}

// COMPONENT MANAGEMENT ----------------------------------------------------------------------------

std::pair<EntityComponentIndex, void*> AddComponent(EntityManager* em,
                                                    EntityID id,
                                                    EEntityComponentType component_type,
                                                    const void* initial_values) {
    auto* signature = GetEntitySignature(em, id);
    if (!signature) {
        return {NONE, nullptr};
    }

    // If it already has the component, we return false.
    if (Matches(*signature, component_type)) {
        return {NONE, nullptr};
    }

    EntityComponentIndex out_index = NONE;
    void* out_component = nullptr;

    Entity* entity = &em->Entities[id.GetIndex()];

    // X-macro to find the component holder.
#define X(component_enum_name, component_struct_name, ...)                            \
    case EEntityComponentType::component_enum_name: {                                 \
        auto [index, component] = em->component_enum_name##ComponentHolder.AddEntity( \
            id,                                                                       \
            entity,                                                                   \
            (const component_struct_name*)initial_values);                            \
        out_index = index;                                                            \
        out_component = component;                                                    \
        break;                                                                        \
    }

    switch (component_type) {
        COMPONENT_TYPES(X)
        default:
            ASSERTF(false, "Unknown component type %d", (u8)component_type);
            return {NONE, nullptr};
    }
#undef X

    entity_manager_private::AddComponentToSignature(signature, component_type);
    return {out_index, out_component};
}

bool HasComponent(const EntityManager& em, EntityID id, EEntityComponentType component_type) {
    auto* signature = GetEntitySignature(em, id);
    if (!signature) {
        return false;
    }

    if (!Matches(*signature, component_type)) {
        return false;
    }

    return true;
}

EntityComponentIndex GetComponent(EntityManager* em,
                                  EntityID id,
                                  EEntityComponentType component_type,
                                  void** out) {
    auto* signature = GetEntitySignature(em, id);
    if (!signature) {
        return NONE;
    }

    if (!Matches(*signature, component_type)) {
        return NONE;
    }

    EntityComponentIndex out_index = NONE;

    // X-macro to find the component holder.
#define X(component_enum_name, ...)                                 \
    case EEntityComponentType::component_enum_name: {               \
        auto [component_index, component_ptr] =                     \
            em->component_enum_name##ComponentHolder.GetEntity(id); \
        if (out) {                                                  \
            *out = component_ptr;                                   \
        }                                                           \
        out_index = component_index;                                \
        break;                                                      \
    }

    switch (component_type) {
        COMPONENT_TYPES(X)
        default: ASSERTF(false, "Unknown component type %d", (u8)component_type); return false;
    }
#undef X

    entity_manager_private::AddComponentToSignature(signature, component_type);
    return out_index;
}

bool RemoveComponent(EntityManager* em, EntityID id, EEntityComponentType component_type) {
    auto* signature = GetEntitySignature(em, id);
    if (!signature) {
        return false;
    }

    // If it doesn't have the component, we return false.
    if (!Matches(*signature, component_type)) {
        return false;
    }

    // X-macro to find the component holder.
#define X(component_enum_name, ...)                                \
    case EEntityComponentType::component_enum_name:                \
        em->component_enum_name##ComponentHolder.RemoveEntity(id); \
        break;

    switch (component_type) {
        COMPONENT_TYPES(X)
        default: ASSERTF(false, "Unknown component type %d", (u8)component_type); return false;
    }
#undef X

    entity_manager_private::RemoveComponentFromSignature(signature, component_type);
    return true;
}

i32 GetComponentCount(const EntityManager& em, EEntityComponentType component_type) {
    ASSERT(component_type < EEntityComponentType::COUNT);

    // X-macro to find the component holder.
#define X(component_enum_name, ...)                                    \
    case EEntityComponentType::component_enum_name:                    \
        return em.component_enum_name##ComponentHolder.ComponentCount; \
        break;

    switch (component_type) {
        COMPONENT_TYPES(X)
        default: ASSERTF(false, "Unknown component type %d", (u8)component_type); return 0;
    }
#undef X
}

std::span<EntityID> GetEntitiesWithComponent(EntityManager* em,
                                             EEntityComponentType component_type) {
    ASSERT(component_type < EEntityComponentType::COUNT);

    // X-macro to find the component holder.
#define X(component_enum_name, ...)                                     \
    case EEntityComponentType::component_enum_name:                     \
        return em->component_enum_name##ComponentHolder.ActiveEntities; \
        break;

    switch (component_type) {
        COMPONENT_TYPES(X)
        default: ASSERTF(false, "Unknown component type %d", (u8)component_type); return {};
    }
#undef X
}

// TEMPLATE IMPLEMENTATION -------------------------------------------------------------------------

template <typename T, i32 SIZE>
void EntityComponentHolder<T, SIZE>::Init(EntityManager* em) {
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
        ComponentToEntity[i] = {i + 1};
    }
    ComponentToEntity.back() = {};

    ASSERT(Owner == nullptr);
    Owner = em;

    SDL_Log("Initialized ComponentHolder: %s\n", ToString(component_type));
}

template <typename T, i32 SIZE>
void EntityComponentHolder<T, SIZE>::Recalculate(EntityManager* em) {
    // Start from the back adjusting the chain.
    for (i32 i = SIZE - 1; i >= 0; i--) {
        EntityID id = ComponentToEntity[i];

        // If the owning entity is valid, we skip it.
        if (IsValid(*em, id)) {
            continue;
        }

        // This component is dead, we add it to the free list.
        ComponentToEntity[i] = {NextComponent};
        NextComponent = i;
    }
}

// Placeholder function to make the compiler happy. Should never be called.
void OnLoadedOnEntity(Entity*, Entity*) { ASSERT(false); }

template <typename T>
constexpr bool HasOnLoadedEntityV =
    requires(::kdk::Entity* entity, T* ptr) { ::kdk::OnLoadedOnEntity(entity, ptr); };

template <typename T, i32 SIZE>
std::pair<EntityComponentIndex, T*>
EntityComponentHolder<T, SIZE>::AddEntity(EntityID id, Entity* entity, const T* initial_values) {
    i32 entity_index = id.GetIndex();

    ASSERT(entity_index >= 0 && entity_index < kMaxEntities);
    ASSERT(ComponentCount < SIZE);
    ASSERT(EntityToComponent[entity_index] == NONE);
    ASSERT(!ActiveEntities.Contains(id));

    // Find the next empty entity.
    EntityComponentIndex component_index = NextComponent;
    ASSERT(component_index != NONE);

    // Update the translation arrays.
    NextComponent = ComponentToEntity[component_index].RawValue;
    ComponentToEntity[component_index] = id;
    EntityToComponent[entity_index] = component_index;
    ActiveEntities.Push(id);

    ComponentCount++;

    // Reset the component.
    T* component = &Components[component_index];
    if (initial_values) {
        *component = *initial_values;
    }

    // Set the bookkeeping values.
    component->_OwnerID = id;
    component->_ComponentIndex = component_index;

    if constexpr (HasOnLoadedEntityV<T>) {
        ::kdk::OnLoadedOnEntity(entity, component);
    }

    return {component_index, component};
}

template <typename T, i32 SIZE>
std::pair<EntityComponentIndex, T*> EntityComponentHolder<T, SIZE>::GetEntity(EntityID id) {
    i32 entity_index = id.GetIndex();

    ASSERT(entity_index >= 0 && entity_index < kMaxEntities);
    ASSERT(ComponentCount > 0);
    EntityComponentIndex component_index = EntityToComponent[entity_index];
    ASSERT(component_index != NONE);

    T* component = &Components[component_index];
    ASSERT(component->_OwnerID == id);
    ASSERT(component->_ComponentIndex == component_index);
    return {component_index, component};
}

template <typename T, i32 SIZE>
void EntityComponentHolder<T, SIZE>::RemoveEntity(EntityID id) {
    i32 entity_index = id.GetIndex();

    ASSERT(entity_index >= 0 && entity_index < kMaxEntities);
    ASSERT(ComponentCount > 0);
    ASSERT(ActiveEntities.Contains(id));

    // Get the component index for this entity
    EntityComponentIndex component_index = EntityToComponent[entity_index];
    ASSERT(component_index != NONE);

    // Update the translation arrays.
    EntityToComponent[entity_index] = NONE;
    ComponentToEntity[component_index] = {NextComponent};
    NextComponent = component_index;
    ActiveEntities.Remove(id);

    ComponentCount--;
}

}  // namespace kdk

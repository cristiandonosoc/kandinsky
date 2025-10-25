#include <kandinsky/entity_manager.h>

#include <kandinsky/core/file.h>
#include <kandinsky/platform.h>

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
        em->EntityData.Signatures[i] = i + 1;
    }

    // Init the component holders.
#define X(component_enum_name, ...)                        \
    case EEntityComponentType::component_enum_name:        \
        em->component_enum_name##ComponentHolder.Init(em); \
        break;

    for (u8 i = 0; i < (u8)EEntityComponentType::COUNT; i++) {
        switch ((EEntityComponentType)i) {
            COMPONENT_TYPES(X)
            case EEntityComponentType::COUNT: ASSERT(false); break;
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
            case EEntityComponentType::COUNT: ASSERT(false); break;
        }
    }
#undef X

    for (u32 i = 0; i < kMaxEntities; ++i) {
        em->EntityData.Signatures[i] = NONE;
    }
}

void Start(EntityManager* em) {
    // Because we bind a pointer from the typed entity to the base entity, we need to "update" it
    // to point to the right place.
    // This is because of caching pointers. Maybe we should just use ids for everything and forget
    // this nonsense.
    //
    // TODO(cdc): Evaluate if this is worth the hassle. For now this is pretty fast.
    for (i32 i = 0; i < kMaxEntities; i++) {
        Entity* entity = &em->EntityData.Entities[i];
        EntityTypeWrapper::OpaqueEntity* opaque =
            &em->EntityData.EntityTypeWrappers[i]._OpaqueEntity;
        opaque->OwningEntity = entity;
    }
}

template <typename T>
constexpr bool EntityHasUpdateV =
    requires(Entity* entity, T* ptr, float dt) { ::kdk::Update(ptr, dt); };

// We require this template because all arms of a constexpr evaluation are evaluated in compile
// time, even if they evaluate to false. Meaning that I cannot call functions that would not exist,
// though I'm sure I use this trick somewhere else, so it might be that `EntityHasUpdateV` behaves
// incorrectly with constexpr.
//
// Having this function permits me to workaround this and get the thing to compile and only call
// update if it is defined for the entity.
template <typename T>
void UpdateTypedEntity_Internal(T* typed_entity, float dt) {
    Update(typed_entity, dt);
}

void Update(EntityManager* em, float dt) {
    (void)em;
    (void)dt;

#define X(ENUM_NAME, STRUCT_NAME, ...)                                                \
    if constexpr (EntityHasUpdateV<STRUCT_NAME>) {                                    \
        for (EntityID id : em->EntityData.Entity_##ENUM_NAME##_Alive) {               \
            STRUCT_NAME* typed_entity =                                               \
                &em->EntityData.EntityTypeWrappers[id.GetIndex()].ENUM_NAME##_Entity; \
            UpdateTypedEntity_Internal(typed_entity, dt);                             \
        }                                                                             \
    }

    ENTITY_TYPES(X)
#undef X
}

void Recalculate(EntityManager* em) {
    em->NextIndex = 0;
    // We go from back to front adjusting the chain.
    for (i16 i = kMaxEntities - 1; i >= 0; i--) {
        // If the entity is alive, we skip it.
        if (IsLive(em->EntityData.Signatures[i])) {
            continue;
        }

        // This entity is dead, we add it to the free list.
        em->EntityData.Signatures[i] = em->NextIndex;
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
            case EEntityComponentType::COUNT: ASSERT(false); break;
        }
    }
#undef X
}

// SERIALIZATION -----------------------------------------------------------------------------------

void Serialize(SerdeArchive* sa, EntityManager* em) {
    SERDE(sa, em, NextIndex);

    if (sa->Mode == ESerdeMode::Serialize) {
        SERDE(sa, em, EntityCount);

        auto entities = NewDynArray<Entity>(sa->TempArena, em->EntityCount);

        VisitAllEntities(em, [&entities](EntityID, Entity* entity) {
            // TODO(cdc): Have a way to avoid copying everything just for serializing.
            entities.Push(*entity);
            return true;
        });
        Serde(sa, "Entities", &entities);
    } else {
        Init(em, {.Recalculate = false});

        i32 incoming_entity_count = NONE;
        Serde(sa, "EntityCount", &incoming_entity_count);

        auto entities = NewDynArray<Entity>(sa->TempArena, incoming_entity_count);
        Serde(sa, "Entities", &entities);
        ASSERT(em->EntityCount == incoming_entity_count);
        ASSERT(em->EntityCount == entities.Size);

        // Now that we have the components, we can recalculate the entity manager.
        Recalculate(em);
    }
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

    Entity* entity = &em->EntityData.Entities[id.GetIndex()];

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
        case EEntityComponentType::COUNT: {
            ASSERT(false);
            return {NONE, nullptr};
        }
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
        case EEntityComponentType::COUNT: ASSERT(false); return false;
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
        case EEntityComponentType::COUNT: ASSERT(false); break;
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
        case EEntityComponentType::COUNT: ASSERT(false); return 0;
    }
#undef X

    ASSERT(false);
    return 0;
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
        case EEntityComponentType::COUNT: ASSERT(false); return {};
    }
#undef X

    ASSERT(false);
    return {};
}

void CalculateModelMatrices(EntityManager* em) {
    // TODO(cdc): Would it be faster to just calculate them all always?
    //            This sounds parallelizable...
    i32 found_count = 0;
    for (i32 i = 0; i < kMaxEntities; i++) {
        if (IsLive(em->EntityData.Signatures[i])) {
            Entity& entity = em->EntityData.Entities[i];
            CalculateModelMatrix(entity.Transform, &em->EntityData.ModelMatrices[i]);
            found_count++;
        }

        if (found_count >= em->EntityCount) {
            break;
        }
    }
}

// ARCHETYPE ---------------------------------------------------------------------------------------

void Serialize(SerdeArchive* sa, Archetype* archetype) {
    SERDE(sa, archetype, Name);
    SERDE(sa, archetype, EntityType);
    SERDE(sa, archetype, EntityFlags);

#define X(ENUM_NAME, STRUCT_NAME, ...)                                     \
    case EEntityType::ENUM_NAME: {                                         \
        Serde(sa, #ENUM_NAME, &archetype->TypedEntity.ENUM_NAME##_Entity); \
        break;                                                             \
    }

    switch (archetype->EntityType) {
        ENTITY_TYPES(X)
        case EEntityType::Invalid: ASSERT(false); break;
        case EEntityType::COUNT: ASSERT(false); break;
    }
#undef X
}

void Serialize(SerdeArchive* sa, ArchetypeManager* am) { SERDE(sa, am, Archetypes); }

bool LoadArchetypes(PlatformState* ps, const String& path) {
    ASSERT(ps->EditorState.RunningMode == ERunningMode::Editor);

    auto data = LoadFile(&ps->Memory.FrameArena, path, {.NullTerminate = false});
    if (data.empty()) {
        SDL_Log("Empty file read in %s", path.Str());
        return false;
    }

    ResetStruct(&ps->Archetypes);
    SerdeArchive sa = NewSerdeArchive(&ps->Memory.PermanentArena,
                                      &ps->Memory.FrameArena,
                                      ESerdeBackend::YAML,
                                      ESerdeMode::Deserialize);
    Load(&sa, data);

    SerdeContext sc = {};
    FillSerdeContext(ps, &sc);
    SetSerdeContext(&sa, &sc);
    Serde(&sa, "Archetypes", &ps->Archetypes);

    return true;
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
    ComponentToEntity.Last() = {};

    ASSERT(Owner == nullptr);
    Owner = em;

    SDL_Log("Initialized ComponentHolder: %s\n", ToString(component_type).Str());
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

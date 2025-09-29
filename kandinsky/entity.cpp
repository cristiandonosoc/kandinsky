#include <imgui.h>
#include <kandinsky/entity.h>

#include <kandinsky/core/defines.h>
#include <kandinsky/core/intrin.h>
#include <kandinsky/core/math.h>
#include <kandinsky/core/serde.h>
#include <kandinsky/entity_manager.h>
#include <kandinsky/imgui.h>
#include <kandinsky/imgui_widgets.h>
#include <kandinsky/platform.h>

#include <SDL3/SDL.h>
#include <glm/common.hpp>
#include <glm/gtc/quaternion.hpp>
#include <limits>

namespace kdk {

namespace entity_private {

bool consteval EntityTypeCountAreLessThanMax() {
    i32 count = 0;
#define X(ENUM_NAME, STRUCT_NAME, MAX_COUNT) count += MAX_COUNT;
    ENTITY_TYPES(X)
#undef X

    return count <= kMaxEntities;
}

static_assert(EntityTypeCountAreLessThanMax(),
              "The total count of all entity types must be less than kMaxEntities! Check entity.h");

}  // namespace entity_private

EntityManager* GetRunningEntityManager() { return platform::GetPlatformContext()->EntityManager; }

const char* ToString(EEntityType entity_type) {
#define X(name, ...) \
    case EEntityType::name: return #name;

    switch (entity_type) {
        ENTITY_TYPES(X)
        case EEntityType::Invalid: return "<invalid>";
        case EEntityType::COUNT: {
            ASSERT(false);
            return "<count>";
        }
    }

#undef X

    ASSERTF(false, "Unknown entity type %d", (u8)entity_type);
    return "<unknown>";
}

bool Matches(const EntitySignature& signature, EEntityComponentType component_type) {
    ASSERT((u8)component_type < kMaxComponentTypes);
    u8 offset = 1 << (u8)component_type;
    return (signature & offset) != 0;
}

const char* ToString(EEntityComponentType component_type) {
    // X-macro to find the component holder.
#define X(component_enum_name, ...) \
    case EEntityComponentType::component_enum_name: return #component_enum_name;

    switch (component_type) {
        COMPONENT_TYPES(X)
        default:
            ASSERTF(false, "Unknown component type %d", (u8)component_type);
            return "<invalid>";
    }
#undef X
}

std::pair<EntityID, Entity*> CreateEntity(EntityManager* em,
                                          EEntityType entity_type,
                                          const CreateEntityOptions& options,
                                          const void* initial_values) {
    ASSERT(em->EntityCount < kMaxEntities);
    ASSERT(entity_type > EEntityType::Invalid && entity_type < EEntityType::COUNT);

    EntityID id = {};
    i32 new_entity_index = NONE;

    if (options._Advanced_OverrideID == NONE) {
        // Find the next empty entity.
        new_entity_index = em->NextIndex;
        ASSERT(new_entity_index != NONE);

        // Negative signatures means that the entity is alive.
        EntitySignature& new_entity_signature = em->EntityData.Signatures[new_entity_index];
        ASSERT(new_entity_signature >= 0);
        em->NextIndex = new_entity_signature;
        new_entity_signature = kNewEntitySignature;

        // Update the next entity pointer.

        auto& new_entity_generation = em->EntityData.Generations[new_entity_index];
        new_entity_generation++;

        ASSERT(new_entity_index < std::numeric_limits<i16>::max());
        id = EntityID::Build((i16)new_entity_index, new_entity_generation, entity_type);
    } else {
        // With this we are explicitly overriding the new entity index/generation.
        // By itself this it not so bad, but it does mess big time with the new "next index"
        // algorithm.
        //
        // We assume this is ok because of the "advanced" use case, which most likely is the whole
        // EntityManager being loaded from file, in which case it will have the NextIndex and other
        // state correctly restored from disk.
        id = options._Advanced_OverrideID;

        // Override the index.
        new_entity_index = options._Advanced_OverrideID.GetIndex();
        ASSERT(new_entity_index != NONE);

        // Negative signatures means that the entity is alive.
        EntitySignature& new_entity_signature = em->EntityData.Signatures[new_entity_index];
        ASSERT(new_entity_signature >= 0);
        new_entity_signature = kNewEntitySignature;

        // Override the geneartion.
        em->EntityData.Generations[new_entity_index] = options._Advanced_OverrideID.GetGeneration();
    }

    // Reset the entity data.
    Entity& entity = em->EntityData.Entities[new_entity_index];
    entity = {
        .ID = id,
        .Name = options.Name,
        .Transform = options.Transform,
    };
    em->EntityCount++;

    // Add it to the correct entity alive list and correctly set the wrapper.
#define X(ENUM_NAME, STRUCT_NAME, ...)                                                        \
    case EEntityType::ENUM_NAME: {                                                            \
        ASSERT(!em->EntityData.Entity_##ENUM_NAME##_Alive.Contains(id));                      \
        em->EntityData.Entity_##ENUM_NAME##_Alive.Push(id);                                   \
        auto& typed = em->EntityData.EntityTypeWrappers[new_entity_index].ENUM_NAME##_Entity; \
        if (!initial_values) {                                                                \
            ResetStruct(&typed);                                                              \
        } else {                                                                              \
            typed = *(STRUCT_NAME*)initial_values;                                            \
        }                                                                                     \
        typed._EntityID = id;                                                                 \
        break;                                                                                \
    }

    switch (entity_type) {
        ENTITY_TYPES(X)
        case EEntityType::Invalid: ASSERT(false); break;
        case EEntityType::COUNT: ASSERT(false); break;
    }
#undef X

    return {id, &entity};
}

void DestroyEntity(EntityManager* em, EntityID id) {
    ASSERT(id != NONE);

    i32 index = id.GetIndex();
    ASSERT(index >= 0 && index < kMaxEntities);

    // Positive signatures means that the entity is not alive (and this slot is pointing to a empty
    // slot).
    EntitySignature signature = em->EntityData.Signatures[index];
    if (!IsLive(signature)) {
        return;
    }
    ASSERT(em->EntityData.Signatures[index] != NONE);

    // Since this is a live entity, we compare generations.
    u8 generation = id.GetGeneration();
    if (generation != em->EntityData.Generations[index]) {
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
        RemoveComponent(em, id, component_type);
        signature_bitfield &= signature_bitfield - 1;  // Clear the lowest bit.
    }

    // Mark the destroyed entity as the next (so we will fill that slot first).
    // We also mark that slot pointing to the prev next entity.
    em->EntityData.Signatures[index] = em->NextIndex;
    em->NextIndex = index;

    // TODO(cdc): We don't need to clear in release builds.
    em->EntityData.Entities[index] = {};

    // Remove it from the alive list.
#define X(ENUM_NAME, ...)                                                           \
    case EEntityType::ENUM_NAME: {                                                  \
        auto [found_index, _] = em->EntityData.Entity_##ENUM_NAME##_Alive.Find(id); \
        ASSERT(found_index != NONE);                                                \
        em->EntityData.Entity_##ENUM_NAME##_Alive.RemoveUnorderedAt(found_index);   \
        break;                                                                      \
    }
    switch (id.GetEntityType()) {
        ENTITY_TYPES(X)
        case EEntityType::Invalid: ASSERT(false); break;
        case EEntityType::COUNT: ASSERT(false); break;
    }
#undef X

    em->EntityCount--;
}

std::pair<EntityID, Entity*> CloneEntity(EntityManager* em, EntityID id) {
    if (!IsValid(*em, id)) {
        return {{}, nullptr};
    }

    Entity* entity = GetEntity(em, id);
    ASSERT(entity);

    auto [new_id, new_entity] = CreateEntity(em,
                                             entity->GetEntityType(),
                                             {
                                                 .Transform = entity->Transform,
                                             });
    ASSERT(new_id != NONE);

#define X(ENUM_NAME, STRUCT_NAME, ...)                                         \
    case EEntityComponentType::ENUM_NAME: {                                    \
        auto [component_index, component] = GetComponent<STRUCT_NAME>(em, id); \
        ASSERT(component);                                                     \
        auto [new_component_index, new_component] =                            \
            AddComponent<STRUCT_NAME>(em, new_id, component);                  \
        ASSERT(new_component);                                                 \
        break;                                                                 \
    }

    // Go over all components and clone them.
    EntitySignature signature = em->EntityData.Signatures[id.GetIndex()];
    ASSERT(IsLive(signature));

    i32 signature_bitfield = (i32)signature;
    while (signature_bitfield) {
        // Get the component type from the signature.
        EEntityComponentType component_type =
            (EEntityComponentType)BitScanForward(signature_bitfield);
        if (component_type >= EEntityComponentType::COUNT) {
            break;
        }

        switch (component_type) {
            COMPONENT_TYPES(X)
            case EEntityComponentType::COUNT: ASSERT(false); break;
        }

#undef X

        signature_bitfield &= signature_bitfield - 1;  // Clear the lowest bit.
    }

    return {new_id, new_entity};
}

void* GetTypedEntityOpaque(EntityManager* em, EntityID id) {
    ASSERT(IsValid(*em, id));

#define X(ENUM_NAME, ...)                                                            \
    case EEntityType::ENUM_NAME: {                                                   \
        return &em->EntityData.EntityTypeWrappers[id.GetIndex()].ENUM_NAME##_Entity; \
    }

    switch (id.GetEntityType()) {
        ENTITY_TYPES(X)
        case EEntityType::Invalid: ASSERT(false); break;
        case EEntityType::COUNT: ASSERT(false); break;
    }
#undef X

    return nullptr;
}

bool IsValid(const EntityManager& em, EntityID id) {
    if (id == NONE) {
        return false;
    }

    i32 index = id.GetIndex();
    ASSERT(index >= 0 && index < kMaxEntities);

    // Live entities have a negative signature.
    EntitySignature signature = em.EntityData.Signatures[index];
    if (!IsLive(signature)) {
        return false;
    }

    // We simply compare generations.
    u8 generation = id.GetGeneration();
    if (em.EntityData.Generations[index] != generation) {
        return false;
    }

    return true;
}

EntitySignature* GetEntitySignature(EntityManager* em, EntityID id) {
    const auto* const_em = const_cast<const EntityManager*>(em);
    return const_cast<EntitySignature*>(GetEntitySignature(*const_em, id));
}

const EntitySignature* GetEntitySignature(const EntityManager& em, EntityID id) {
    if (!IsValid(em, id)) {
        return nullptr;
    }

    i32 index = id.GetIndex();
    ASSERT(index >= 0 && index < kMaxEntities);
    return &em.EntityData.Signatures[index];
}

Entity* GetEntity(EntityManager* em, EntityID id) {
    return const_cast<Entity*>(GetEntity(*em, id));
}

void VisitEntitiesOpaque(EntityManager* em,
                         EEntityType entity_type,
                         const kdk::Function<bool(EntityID, Entity*, void*)>& visitor) {
#define X(ENUM_NAME, STRUCT_NAME, ...)                                                \
    case EEntityType::ENUM_NAME: {                                                    \
        for (EntityID id : em->EntityData.Entity_##ENUM_NAME##_Alive) {               \
            Entity* entity = GetEntity(em, id);                                       \
            ASSERT(entity);                                                           \
            STRUCT_NAME* typed =                                                      \
                &em->EntityData.EntityTypeWrappers[id.GetIndex()].ENUM_NAME##_Entity; \
            if (!visitor(id, entity, typed)) [[unlikely]] {                           \
                break;                                                                \
            }                                                                         \
        }                                                                             \
        break;                                                                        \
    }

    switch (entity_type) {
        ENTITY_TYPES(X)
        case EEntityType::Invalid: ASSERT(false); break;
        case EEntityType::COUNT: ASSERT(false); break;
    }

#undef X
}

const Entity* GetEntity(const EntityManager& em, EntityID id) {
    if (!IsValid(em, id)) {
        return nullptr;
    }

    i32 index = id.GetIndex();
    ASSERT(index >= 0 && index < kMaxEntities);
    return &em.EntityData.Entities[index];
}

void VisitAllEntities(EntityManager* em, const kdk::Function<bool(EntityID, Entity*)>& visitor) {
    i32 found = 0;
    for (i32 i = 0; i < kMaxEntities; i++) {
        if (IsLive(em->EntityData.Signatures[i])) {
            Entity& entity = em->EntityData.Entities[i];
            if (!visitor(entity.ID, &entity)) [[unlikely]] {
                break;
            }

            found++;
            if (found >= em->EntityCount) [[unlikely]] {
                break;
            }
        }
    }
}

void UpdateModelMatrices(EntityManager* em) {
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

const Mat4& GetModelMatrix(const EntityManager& em, EntityID id) {
    return em.EntityData.ModelMatrices[id.GetIndex()];
}

// SERIALIZE ---------------------------------------------------------------------------------------

namespace entity_private {

void SerializeTypedEntity(SerdeArchive* sa, EntityManager* em, EntityID id, Entity* entity) {
#define X(ENUM_NAME, STRUCT_NAME, ...)                                  \
    case EEntityType::ENUM_NAME: {                                      \
        if (STRUCT_NAME* typed = GetTypedEntity<STRUCT_NAME>(em, id)) { \
            Serde(sa, #ENUM_NAME, typed);                               \
        }                                                               \
        break;                                                          \
    }

    switch (entity->GetEntityType()) {
        ENTITY_TYPES(X)
        case EEntityType::Invalid: ASSERT(false); break;
        case EEntityType::COUNT: ASSERT(false); break;
    }

#undef X
}

void SerializeComponent(SerdeArchive* sa,
                        EntityManager* em,
                        EntityID id,
                        EEntityComponentType component_type) {
    if (sa->Mode == ESerdeMode::Serialize) {
#define X(component_enum_name, component_type, ...)                               \
    case EEntityComponentType::component_enum_name: {                             \
        auto [component_index, component] = GetComponent<component_type>(em, id); \
        if (component_index != NONE) {                                            \
            Serde(sa, #component_enum_name, component);                           \
        }                                                                         \
        break;                                                                    \
    }

        switch (component_type) {
            COMPONENT_TYPES(X)
            default: ASSERTF(false, "Unknown component type %d", (u8)component_type); return;
        }

#undef X

    } else {
#define X(component_enum_name, component_type, ...)                   \
    case EEntityComponentType::component_enum_name: {                 \
        component_type component{};                                   \
        Serde(sa, #component_enum_name, &component);                  \
        auto [component_index, _] = AddComponent(em, id, &component); \
        ASSERT(component_index != NONE);                              \
        break;                                                        \
    }

        switch (component_type) {
            COMPONENT_TYPES(X)
            default: ASSERTF(false, "Unknown component type %d", (u8)component_type); return;
        }

#undef X
    }
}

}  // namespace entity_private

void Serialize(SerdeArchive* sa, EntityManager* em) {
    using namespace entity_private;

    SERDE(sa, em, NextIndex);

    if (sa->Mode == ESerdeMode::Serialize) {
        SERDE(sa, em, EntityCount);

        auto entities = NewDynArray<Entity>(sa->TempArena, em->EntityCount);

        VisitAllEntities(em, [sa, &entities](EntityID, Entity* entity) {
            // TODO(cdc): Have a way to avoid copying everything just for serializing.
            entities.Push(sa->TempArena, *entity);
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

void Serialize(SerdeArchive* sa, Entity* entity) {
    using namespace entity_private;

    Serde(sa, "ID", &entity->ID.RawValue);

    if (sa->Mode == ESerdeMode::Serialize) {
        auto* signature = GetEntitySignature(sa->SerdeContext->EntityManager, entity->ID);
        ASSERT(signature);
        entity->_Signature = *signature;
    }

    Serde(sa, "Signature", &entity->_Signature);
    SERDE(sa, entity, Name);
    SERDE(sa, entity, Transform);
    // We don't care about the model matrix, since it is calculated on the fly.

    // If we're deserializing, we need to actually create the entity entry in the EntityManager.
    if (sa->Mode == ESerdeMode::Deserialize) {
        // Create the entity in the EntityManager.
        CreateEntityOptions options{
            .Name = entity->Name.ToString(),
            .Transform = entity->Transform,
            ._Advanced_OverrideID = entity->ID,
        };
        auto [id, created_entity] =
            CreateEntity(sa->SerdeContext->EntityManager, entity->GetEntityType(), options);
        ASSERT(id == entity->ID);
        *created_entity = *entity;  // Copy the rest of the data.
    }

    // Serialize the entity type.
    SerializeTypedEntity(sa, sa->SerdeContext->EntityManager, entity->ID, entity);

    // Go over all components and remove them from the entity.
    i32 signature_bitfield = (i32)entity->_Signature;
    while (signature_bitfield) {
        // Get the component type from the signature.
        EEntityComponentType component_type =
            (EEntityComponentType)BitScanForward(signature_bitfield);
        if (component_type >= EEntityComponentType::COUNT) {
            break;
        }

        // Remove the component from the entity.
        SerializeComponent(sa, sa->SerdeContext->EntityManager, entity->ID, component_type);
        signature_bitfield &= signature_bitfield - 1;  // Clear the lowest bit.
    }
}

EntityComponentIndex GetComponentIndex(const EntityManager& em,
                                       EntityID id,
                                       EEntityComponentType component_type) {
    ASSERT(component_type < EEntityComponentType::COUNT);
    if (!IsValid(em, id)) {
        return NONE;
    }

    i32 entity_index = id.GetIndex();
    if (!Matches(em.EntityData.Signatures[entity_index], component_type)) {
        return NONE;  // Entity does not have this component.
    }

    // X-macro to find the component holder.
#define X(component_enum_name, ...)                                       \
    case EEntityComponentType::component_enum_name: {                     \
        auto& component_holder = em.component_enum_name##ComponentHolder; \
        return component_holder.EntityToComponent[entity_index];          \
        break;                                                            \
    }

    switch (component_type) {
        COMPONENT_TYPES(X)
        case EEntityComponentType::COUNT: ASSERT(false); return NONE;
    }
#undef X

    ASSERT(false);
    return NONE;
}

EntityID GetOwningEntity(const EntityManager& em,
                         EEntityComponentType component_type,
                         EntityComponentIndex component_index) {
    ASSERT(component_type < EEntityComponentType::COUNT);

    // X-macro to find the component holder.
#define X(component_enum_name, ...)                                                        \
    case EEntityComponentType::component_enum_name: {                                      \
        auto& component_holder = em.component_enum_name##ComponentHolder;                  \
        ASSERT(component_index >= 0 && component_index < component_holder.kMaxComponents); \
        return component_holder.ComponentToEntity[component_index];                        \
    }

    switch (component_type) {
        COMPONENT_TYPES(X)
        case EEntityComponentType::COUNT: ASSERT(false); break;
    }
#undef X

    ASSERT(false);
    return {};
}

// IMGUI -------------------------------------------------------------------------------------------

namespace entity_private {

String EntityDisplayString(Arena* arena, const Entity& entity, i32 index) {
    // Format the display string
    return Printf(arena,
                  "%04d: %s (Index: %d, Gen: %d) (Type: %s)",
                  index,
                  entity.Name.Str(),
                  entity.ID.GetIndex(),
                  entity.ID.GetGeneration(),
                  ToString(entity.GetEntityType()));
}

}  // namespace entity_private

void BuildEntityListImGui(PlatformState* ps, EntityManager* em) {
    auto scratch = GetScratchArena();

    static ImGuiTextFilter filter;
    filter.Draw("Filter");

    if (ImGui::BeginListBox("Entities",
                            ImVec2(-FLT_MIN, 10 * ImGui::GetTextLineHeightWithSpacing()))) {
        for (i32 i = 0; i < kMaxEntities; i++) {
            if (!IsLive(em->EntityData.Signatures[i])) {
                continue;
            }

            const Entity& entity = em->EntityData.Entities[i];

            String display = entity_private::EntityDisplayString(scratch.Arena, entity, i);
            if (!filter.PassFilter(display.Str())) {
                continue;
            }

            bool is_selected = (ps->SelectedEntityID == entity.ID);
            if (ImGui::Selectable(display.Str(),
                                  is_selected,
                                  ImGuiSelectableFlags_AllowDoubleClick)) {
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    SDL_Log("Double-clicked entity %d", entity.ID.RawValue);
                    SetTargetEntity(ps, entity);
                    // Handle double-click
                } else {
                    SDL_Log("Selected entity %d", entity.ID.RawValue);
                    ps->SelectedEntityID = entity.ID;
                }
            }

            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        // }
        ImGui::EndListBox();
    }
}

void BuildEntityDebuggerImGui(PlatformState* ps, EntityManager* em) {
    (void)ps;

    auto scratch = GetScratchArena();
    String em_size = ToMemoryString(scratch.Arena, sizeof(EntityManager));
    ImGui::Text("EntityManager size: %s", em_size.Str());

    if (ImGui::TreeNodeEx("Memory Detail", ImGuiTreeNodeFlags_Framed)) {
        u32 base_entity_size = sizeof(em->EntityData.Generations) +
                               sizeof(em->EntityData.Signatures) + sizeof(em->EntityData.Entities);
        ImGui::Text("Base Entity: %s", ToMemoryString(scratch.Arena, base_entity_size).Str());

#define X(ENUM_NAME, STRUCT_NAME, MAX_COUNT, ...)                    \
    case EEntityComponentType::ENUM_NAME: {                          \
        u32 elem_size = sizeof(STRUCT_NAME);                         \
        u32 array_size = sizeof(em->ENUM_NAME##ComponentHolder);     \
        ImGui::Text(#ENUM_NAME ": %s (%u * %s)",                     \
                    ToMemoryString(scratch.Arena, array_size).Str(), \
                    MAX_COUNT,                                       \
                    ToMemoryString(scratch, elem_size).Str());       \
        break;                                                       \
    }

        for (u8 i = 0; i < (u8)EEntityComponentType::COUNT; i++) {
            EEntityComponentType component_type = (EEntityComponentType)i;

            switch (component_type) {
                COMPONENT_TYPES(X)
                default: ASSERTF(false, "Unknown component type %d", (u8)component_type); return;
            }
        }
#undef X

        ImGui::TreePop();
    }

    ImGui::Text("Next Free Entity: %d", em->NextIndex);

    if (ImGui::BeginListBox(" ", ImVec2(-FLT_MIN, -FLT_MIN))) {
        ImGuiListClipper clipper;
        clipper.Begin(kMaxEntities);

        while (clipper.Step()) {
            for (i32 i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                auto signature = em->EntityData.Signatures[i];
                if (IsLive(signature)) {
                    const Entity& entity = em->EntityData.Entities[i];
                    String display = entity_private::EntityDisplayString(scratch.Arena, entity, i);
                    ImGui::Selectable(display.Str(), true);
                } else {
                    String display =
                        Printf(scratch.Arena, "%04d: <empty> (next: %d)", i, signature);
                    ImGui::Selectable(display.Str(), false);
                }
            }
        }

        ImGui::EndListBox();
    }
}

template <typename T>
constexpr bool HasBuildImGuiV = requires(T* ptr) { ::kdk::BuildImGui(ptr); };

template <typename T>
void BuildEntityTypeImGui(EntityManager* em, Entity* entity, T* typed) {
    (void)em;
    if constexpr (HasBuildImGuiV<T>) {
        if (ImGui::TreeNodeEx(ToString(entity->GetEntityType()), ImGuiTreeNodeFlags_Framed)) {
            BuildImGui(typed);

            ImGui::TreePop();
        }
    } else {
        auto scratch = GetScratchArena();
        String msg = Printf(scratch.Arena, "%s: No ImGui support", ToString(T::kEntityType));
        ImGui::Text("%s", msg.Str());
    }
}

template <typename T>
void BuildComponentImGui(EntityManager* em, T* component) {
    auto scratch = GetScratchArena();

    if constexpr (HasBuildImGuiV<T>) {
        String label = Printf(scratch.Arena,
                              "%s (Index: %d)",
                              ToString(T::kComponentType),
                              component->GetComponentIndex());
        if (ImGui::TreeNodeEx(label.Str(),
                              ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) {
            BuildImGui(component);

            if (ImGui_DangerButton("Remove Component")) {
                EntityID owner = component->GetOwnerID();
                ASSERT(IsValid(*em, owner));
                bool removed = RemoveComponent(em, owner, T::kComponentType);
                ASSERT(removed);
                SDL_Log("Removed component %s from entity %d",
                        ToString(T::kComponentType),
                        owner.RawValue);
            }

            ImGui::TreePop();
        }
    } else {
        String msg = Printf(scratch.Arena, "%s: No ImGui support", ToString(T::kComponentType));
        ImGui::Text("%s", msg.Str());
    }
}

void BuildImGui(EntityManager* em, EntityID id) {
    if (!IsValid(*em, id)) {
        ImGui::Text("Entity %d: Not valid", id.RawValue);
        return;
    }

    Entity* entity = GetEntity(em, id);
    ASSERT(entity);

    ImGui::Text("ID: %d (Index: %d, Gen: %d) - Type: %s\n",
                id.RawValue,
                id.GetIndex(),
                id.GetGeneration(),
                ToString(entity->GetEntityType()));
    auto* signature = GetEntitySignature(em, id);
    ASSERT(signature);
    BuildImGui_EntitySignature(*signature);

    BuildImGui(&entity->Transform);

    // Add entity type specific ImGui.
    {
#define X(ENUM_NAME, STRUCT_NAME, ...)                            \
    case EEntityType::ENUM_NAME: {                                \
        STRUCT_NAME* typed = GetTypedEntity<STRUCT_NAME>(em, id); \
        if (typed) {                                              \
            BuildEntityTypeImGui(em, entity, typed);              \
        }                                                         \
        break;                                                    \
    }

        switch (entity->GetEntityType()) {
            ENTITY_TYPES(X)
            case EEntityType::Invalid: ASSERT(false); break;
            case EEntityType::COUNT: ASSERT(false); break;
        }

#undef X
    }

    // Add component combo.
    {
        static int selected_index = -1;
        // Reset the selected id if the entity changes.
        static EntityID selected_entity_id = {};
        if (selected_entity_id != id) {
            selected_entity_id = id;
            selected_index = -1;
        }

        FixedVector<EEntityComponentType, (i32)EEntityComponentType::COUNT> available_components;
        FixedVector<const char*, (i32)EEntityComponentType::COUNT> available_component_names;
        for (u8 i = 0; i < (u8)EEntityComponentType::COUNT; i++) {
            EEntityComponentType component_type = (EEntityComponentType)i;
            if (!HasComponent(*em, id, component_type)) {
                available_components.Push(component_type);
                available_component_names.Push(ToString(component_type));
            }
        }

        if (!available_components.IsEmpty()) {
            ImGui::Text("Add Component: ");
            ImGui::SameLine();

            if (ImGui::Combo("##AddComponent",
                             &selected_index,
                             available_component_names.Data,
                             available_component_names.Size)) {
            }

            ImGui::SameLine();
            if (ImGui::Button("Add") && selected_index != -1) {
                // Add the selected component to the entity.
                ASSERT(selected_index >= 0 && selected_index < available_components.Size);
                if (selected_index >= 0 && selected_index < available_components.Size) {
                    EEntityComponentType component_to_add = available_components[selected_index];
                    auto [index, _] = AddComponent(em, id, component_to_add);
                    ASSERT(index != NONE);
                    SDL_Log("Added component %s to entity %d",
                            ToString(component_to_add),
                            id.RawValue);
                }
            }
        }
    }

#define X(component_enum_name, component_type, ...)                               \
    case EEntityComponentType::component_enum_name: {                             \
        auto [component_index, component] = GetComponent<component_type>(em, id); \
        if (component_index != NONE) {                                            \
            BuildComponentImGui(em, component);                                   \
        }                                                                         \
        break;                                                                    \
    }

    for (u8 i = 0; i < (u8)EEntityComponentType::COUNT; i++) {
        EEntityComponentType component_type = (EEntityComponentType)i;

        switch (component_type) {
            COMPONENT_TYPES(X)
            default: ASSERTF(false, "Unknown component type %d", (u8)component_type); return;
        }
    }
#undef X
}  // namespace kdk

template <typename T>
constexpr bool HasBuildGizmosV =
    requires(PlatformState* ps, T* ptr) { ::kdk::BuildGizmos(ps, ptr); };

template <typename T>
void BuildComponentGizmos(PlatformState* ps, T* component) {
    if constexpr (HasBuildGizmosV<T>) {
        BuildGizmos(ps, component);
    }
}

void BuildGizmos(PlatformState* ps, const Camera& camera, EntityManager* em, EntityID id) {
    if (!IsValid(*em, id)) {
        return;
    }

    Entity* entity = GetEntity(em, id);
    ASSERT(entity);

    // Transform gizmo.
    {
        ps->ImGuiState.EntityDraggingPressed = false;
        ps->ImGuiState.EntityDraggingReleased = false;

        Mat4 model(1.0f);
        model = Translate(model, Vec3(entity->Transform.Position));
        if (ImGuizmo::Manipulate(GetPtr(camera.M_View),
                                 GetPtr(camera.M_Proj),
                                 ImGuizmo::TRANSLATE,
                                 ImGuizmo::WORLD,
                                 GetPtr(model))) {
            entity->Transform.Position = model[3];
            if (!ps->ImGuiState.EntityDraggingDown) {
                ps->ImGuiState.EntityDraggingPressed = true;
                ps->ImGuiState.EntityDraggingDown = true;
            }
        } else if (ps->ImGuiState.EntityDraggingDown) {
            if (!MOUSE_DOWN_IMGUI(ps, LEFT)) {
                ps->ImGuiState.EntityDraggingDown = false;
                ps->ImGuiState.EntityDraggingReleased = true;
            }
        }

        // On the frame we pressed, if we have alt-pressed, we clone the current selected actor.
        if (ps->ImGuiState.EntityDraggingPressed) {
            if (ALT_DOWN_IMGUI(ps)) {
                EntityID original_id = ps->SelectedEntityID;
                auto [new_id, new_entity] = CloneEntity(em, original_id);

                SDL_Log("Cloned entity %d to %d", original_id.RawValue, new_id.RawValue);
                SetTargetEntity(ps, *new_entity, {.FocusCamera = false});
            }
        }
    }

#define X(component_enum_name, component_type, ...)                               \
    case EEntityComponentType::component_enum_name: {                             \
        auto [component_index, component] = GetComponent<component_type>(em, id); \
        if (component_index != NONE) {                                            \
            BuildComponentGizmos(ps, component);                                  \
        }                                                                         \
        break;                                                                    \
    }

    for (u8 i = 0; i < (u8)EEntityComponentType::COUNT; i++) {
        EEntityComponentType component_type = (EEntityComponentType)i;

        switch (component_type) {
            COMPONENT_TYPES(X)
            default: ASSERTF(false, "Unknown component type %d", (u8)component_type); return;
        }
    }
#undef X
}

// TEST COMPONENTS ---------------------------------------------------------------------------------

void Serialize(SerdeArchive* sa, TestComponent* tc) { SERDE(sa, tc, Value); }

void Serialize(SerdeArchive* sa, Test2Component* tc) {
    SERDE(sa, tc, Name);
    SERDE(sa, tc, Transform);
}

}  // namespace kdk

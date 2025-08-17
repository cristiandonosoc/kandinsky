#include <kandinsky/entity.h>

#include <kandinsky/core/defines.h>
#include <kandinsky/core/intrin.h>
#include <kandinsky/core/math.h>
#include <kandinsky/core/serde.h>
#include <kandinsky/graphics/light.h>
#include <kandinsky/graphics/model.h>
#include <kandinsky/imgui.h>
#include <kandinsky/imgui_widgets.h>
#include <kandinsky/platform.h>

#include <SDL3/SDL.h>
#include <glm/gtc/quaternion.hpp>

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

const char* ToString(EEntityType entity_type) {
    switch (entity_type) {
        case EEntityType::Invalid: return "<invalid>";
        case EEntityType::Player: return "Player";
        case EEntityType::Enemy: return "Enemy";
        case EEntityType::NPC: return "NPC";
        case EEntityType::Item: return "Item";
        case EEntityType::Projectile: return "Projectile";
        case EEntityType::PointLight: return "PointLight";
        case EEntityType::DirectionalLight: return "DirectionalLight";
        case EEntityType::Spotlight: return "Spotlight";
        case EEntityType::COUNT: {
            ASSERT(false);
            return "<count>";
        }
    }

    ASSERTF(false, "Unknown entity type %d", (u8)entity_type);
    return "<unknown>";
}

template <typename T, i32 SIZE>
struct EntityComponentHolder {
    static constexpr i32 kMaxComponents = SIZE;

    std::array<EntityComponentIndex, kMaxEntities> EntityToComponent;
    std::array<EntityID, SIZE> ComponentToEntity;
    std::array<T, SIZE> Components = {};
    FixedArray<EntityID, SIZE> ActiveEntities;
    EntityManager* Owner = nullptr;
    EntityComponentIndex NextComponent = 0;
    i32 ComponentCount = 0;

    void Init(EntityManager* entity_manager);
    void Shutdown() {}

    std::pair<EntityComponentIndex, T*> AddEntity(EntityID id,
                                                  Entity* entity,
                                                  const T* initial_values = nullptr);
    std::pair<EntityComponentIndex, T*> GetEntity(EntityID id);
    void RemoveEntity(EntityID id);
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
    ASSERT((u8)component_type < kMaxComponentTypes);
    u8 offset = 1 << (u8)component_type;
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
    eem->NextIndex = 0;

    // Empty entities point to the *next* empty entity.
    // NOTE: The last entity points to an invalid slot.
    for (u32 i = 0; i < kMaxEntities; ++i) {
        eem->Signatures[i] = i + 1;
    }

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

std::pair<EntityID, Entity*> CreateEntity(EntityManager* eem, const CreateEntityOptions& options) {
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
    EntityID id = EntityID::Build(new_entity_index, new_entity_generation);

    // Reset the entity data.
    Entity& entity = eem->Entities[new_entity_index];
    entity = {
        .ID = id,
        .EntityType = options.EntityType,
        .Name = options.Name,
        .Transform = options.Transform,
    };

    return {id, &entity};
}

void DestroyEntity(EntityManager* eem, EntityID id) {
    ASSERT(id != NONE);

    i32 index = id.GetIndex();
    ASSERT(index >= 0 && index < kMaxEntities);

    // Positive signatures means that the entity is not alive (and this slot is pointing to a empty
    // slot).
    EntitySignature signature = eem->Signatures[index];
    if (!IsLive(signature)) {
        return;
    }
    ASSERT(eem->Signatures[index] != NONE);

    // Since this is a live entity, we compare generations.
    u8 generation = id.GetGeneration();
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
        RemoveComponent(eem, id, component_type);
        signature_bitfield &= signature_bitfield - 1;  // Clear the lowest bit.
    }

    // Mark the destroyed entity as the next (so we will fill that slot first).
    // We also mark that slot pointing to the prev next entity.
    eem->Signatures[index] = eem->NextIndex;
    eem->NextIndex = index;

    // TODO(cdc): We don't need to clear in release builds.
    eem->Entities[index] = {};

    eem->EntityCount--;
}

bool IsValid(const EntityManager& eem, EntityID id) {
    if (id == NONE) {
        return false;
    }

    i32 index = id.GetIndex();
    ASSERT(index >= 0 && index < kMaxEntities);

    // Live entities have a negative signature.
    EntitySignature signature = eem.Signatures[index];
    if (!IsLive(signature)) {
        return false;
    }

    // We simply compare generations.
    u8 generation = id.GetGeneration();
    if (eem.Generations[index] != generation) {
        return false;
    }

    return true;
}

EntitySignature* GetEntitySignature(EntityManager* eem, EntityID id) {
    if (!IsValid(*eem, id)) {
        return nullptr;
    }

    i32 index = id.GetIndex();
    ASSERT(index >= 0 && index < kMaxEntities);
    return &eem->Signatures[index];
}

Entity* GetEntity(EntityManager* eem, EntityID id) {
    return const_cast<Entity*>(GetEntity(*eem, id));
}

const Entity* GetEntity(const EntityManager& eem, EntityID id) {
    if (!IsValid(eem, id)) {
        return nullptr;
    }

    i32 index = id.GetIndex();
    ASSERT(index >= 0 && index < kMaxEntities);
    return &eem.Entities[index];
}

void VisitEntities(EntityManager* eem, const kdk::Function<bool(EntityID, Entity*)>& visitor) {
    i32 found = 0;
    for (i32 i = 0; i < kMaxEntities; i++) {
        if (IsLive(eem->Signatures[i])) {
            Entity& entity = eem->Entities[i];
            if (!visitor(entity.ID, &entity)) {
                break;
            }

            found++;
            if (found >= eem->EntityCount) {
                break;
            }
        }
    }
}

void UpdateModelMatrices(EntityManager* eem) {
    // TODO(cdc): Would it be faster to just calculate them all always?
    //            This sounds parallelizable...
    i32 found_count = 0;
    for (i32 i = 0; i < kMaxEntities; i++) {
        if (IsLive(eem->Signatures[i])) {
            Entity& entity = eem->Entities[i];
            CalculateModelMatrix(entity.Transform, &entity.M_Model);
            found_count++;
        }

        if (found_count >= eem->EntityCount) {
            break;
        }
    }
}

// SERIALIZE ---------------------------------------------------------------------------------------

namespace entity_private {

void SerializeComponent(SerdeArchive* sa,
                        EntityManager* eem,
                        EntityID id,
                        EEntityComponentType component_type) {
    if (sa->Mode == ESerdeMode::Serialize) {
#define X(component_enum_name, component_type, ...)                                \
    case EEntityComponentType::component_enum_name: {                              \
        auto [component_index, component] = GetComponent<component_type>(eem, id); \
        if (component_index != NONE) {                                             \
            Serde(sa, #component_enum_name, component);                            \
        }                                                                          \
        break;                                                                     \
    }

        switch (component_type) {
            ECS_COMPONENT_TYPES(X)
            default: ASSERTF(false, "Unknown component type %d", (u8)component_type); return;
        }

#undef X

    } else {
#define X(component_enum_name, component_type, ...)                    \
    case EEntityComponentType::component_enum_name: {                  \
        component_type component{};                                    \
        Serde(sa, #component_enum_name, &component);                   \
        auto [component_index, _] = AddComponent(eem, id, &component); \
        ASSERT(component_index != NONE);                               \
        break;                                                         \
    }

        switch (component_type) {
            ECS_COMPONENT_TYPES(X)
            default: ASSERTF(false, "Unknown component type %d", (u8)component_type); return;
        }

#undef X
    }
}

}  // namespace entity_private

void Serialize(SerdeArchive* sa, EntityManager* eem) {
    using namespace entity_private;

    SERDE(sa, eem, NextIndex);
    SERDE(sa, eem, EntityCount);

    sa->Context = eem;
    DEFER { sa->Context = nullptr; };

    if (sa->Mode == ESerdeMode::Serialize) {
        auto entities = NewDynArray<Entity>(sa->Arena, eem->EntityCount);

        VisitEntities(eem, [sa, &entities](EntityID, Entity* entity) {
            // TODO(cdc): Have a way to avoid copying everything just for serializing.
            entities.Push(sa->Arena, *entity);
            return true;
        });
        Serde(sa, "Entities", &entities);
    } else {
        auto entities = NewDynArray<Entity>(sa->Arena, eem->EntityCount);
        Serde(sa, "Entities", &entities);
        ASSERT(entities.Size == (u32)eem->EntityCount);

        // For each of these entities, we need to place the correct place.
        // TODO(cdc): Maybe something could be done to ensure the loading just happens in place.
        //            For now we copy.
        for (u32 i = 0; i < entities.Size; i++) {
            Entity& saved_entity = entities[i];
            u8 generation = saved_entity.ID.GetGeneration();
            i32 index = saved_entity.ID.GetIndex();

            eem->Generations[index] = generation;
            eem->Signatures[index] = saved_entity._Signature;
            eem->Entities[index] = saved_entity;
        }
    }
}

void Serialize(SerdeArchive* sa, Entity* entity) {
    ASSERT(sa->Context);
    EntityManager* eem = (EntityManager*)sa->Context;

    Serde(sa, "ID", &entity->ID.Value);

    if (sa->Mode == ESerdeMode::Serialize) {
        auto* signature = GetEntitySignature(eem, entity->ID);
        ASSERT(signature);
        entity->_Signature = *signature;
    }

    Serde(sa, "Signature", &entity->_Signature);
    Serde(sa, "EntityType", (u8*)&entity->EntityType);
    SERDE(sa, entity, Name);
    SERDE(sa, entity, Transform);

    // We don't care about the model matrix, since it is calculated on the fly.

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
        entity_private::SerializeComponent(sa, eem, entity->ID, component_type);
        signature_bitfield &= signature_bitfield - 1;  // Clear the lowest bit.
    }
}

// COMPONENT MANAGEMENT ----------------------------------------------------------------------------

std::pair<EntityComponentIndex, void*> AddComponent(EntityManager* eem,
                                                    EntityID id,
                                                    EEntityComponentType component_type,
                                                    const void* initial_values) {
    auto* signature = GetEntitySignature(eem, id);
    if (!signature) {
        return {NONE, nullptr};
    }

    // If it already has the component, we return false.
    if (Matches(*signature, component_type)) {
        return {NONE, nullptr};
    }

    EntityComponentIndex out_index = NONE;
    void* out_component = nullptr;

    Entity* entity = &eem->Entities[id.GetIndex()];

    // X-macro to find the component holder.
#define X(component_enum_name, component_struct_name, ...)                                         \
    case EEntityComponentType::component_enum_name: {                                              \
        auto [index, component] = eem->Components->component_enum_name##ComponentHolder.AddEntity( \
            id,                                                                                    \
            entity,                                                                                \
            (const component_struct_name*)initial_values);                                         \
        out_index = index;                                                                         \
        out_component = component;                                                                 \
        break;                                                                                     \
    }

    switch (component_type) {
        ECS_COMPONENT_TYPES(X)
        default:
            ASSERTF(false, "Unknown component type %d", (u8)component_type);
            return {NONE, nullptr};
    }
#undef X

    entity_private::AddComponentToSignature(signature, component_type);
    return {out_index, out_component};
}

EntityComponentIndex GetComponent(EntityManager* eem,
                                  EntityID id,
                                  EEntityComponentType component_type,
                                  void** out) {
    auto* signature = GetEntitySignature(eem, id);
    if (!signature) {
        return NONE;
    }

    if (!Matches(*signature, component_type)) {
        return NONE;
    }

    EntityComponentIndex out_index = NONE;

    // X-macro to find the component holder.
#define X(component_enum_name, ...)                                              \
    case EEntityComponentType::component_enum_name: {                            \
        auto [component_index, component_ptr] =                                  \
            eem->Components->component_enum_name##ComponentHolder.GetEntity(id); \
        if (out) {                                                               \
            *out = component_ptr;                                                \
        }                                                                        \
        out_index = component_index;                                             \
        break;                                                                   \
    }

    switch (component_type) {
        ECS_COMPONENT_TYPES(X)
        default: ASSERTF(false, "Unknown component type %d", (u8)component_type); return false;
    }
#undef X

    entity_private::AddComponentToSignature(signature, component_type);
    return out_index;
}

bool RemoveComponent(EntityManager* eem, EntityID id, EEntityComponentType component_type) {
    auto* signature = GetEntitySignature(eem, id);
    if (!signature) {
        return false;
    }

    // If it doesn't have the component, we return false.
    if (!Matches(*signature, component_type)) {
        return false;
    }

    // X-macro to find the component holder.
#define X(component_enum_name, ...)                                             \
    case EEntityComponentType::component_enum_name:                             \
        eem->Components->component_enum_name##ComponentHolder.RemoveEntity(id); \
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

std::span<EntityID> GetEntitiesWithComponent(EntityManager* eem,
                                             EEntityComponentType component_type) {
    ASSERT(component_type < EEntityComponentType::COUNT);

    // X-macro to find the component holder.
#define X(component_enum_name, ...)                                                  \
    case EEntityComponentType::component_enum_name:                                  \
        return eem->Components->component_enum_name##ComponentHolder.ActiveEntities; \
        break;

    switch (component_type) {
        ECS_COMPONENT_TYPES(X)
        default: ASSERTF(false, "Unknown component type %d", (u8)component_type); return {};
    }
#undef X
}

EntityComponentIndex GetComponentIndex(const EntityManager& eem,
                                       EntityID id,
                                       EEntityComponentType component_type) {
    ASSERT(component_type < EEntityComponentType::COUNT);
    if (!IsValid(eem, id)) {
        return NONE;
    }

    i32 entity_index = id.GetIndex();
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

EntityID GetOwningEntity(const EntityManager& eem,
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
        default: ASSERTF(false, "Unknown component type %d", (u8)component_type); return {};
    }
#undef X
}

void BuildEntityListImGui(PlatformState* ps, EntityManager* eem) {
    auto scratch = GetScratchArena();
    String eem_size = ToMemoryString(sizeof(EntityManager));
    ImGui::Text("EntityManager size: %s", eem_size.Str());

    static ImGuiTextFilter filter;
    filter.Draw("Filter");

    if (ImGui::BeginListBox("Entities",
                            ImVec2(-FLT_MIN, 10 * ImGui::GetTextLineHeightWithSpacing()))) {
        for (i32 i = 0; i < kMaxEntities; i++) {
            if (!IsLive(eem->Signatures[i])) {
                continue;
            }

            const Entity& entity = eem->Entities[i];

            // Format the display string
            String display = Printf(scratch.Arena,
                                    "%04d: %s (Index: %d, Gen: %d) (Type: %s)",
                                    i,
                                    entity.Name.Str(),
                                    entity.ID.GetIndex(),
                                    entity.ID.GetGeneration(),
                                    ToString(entity.EntityType));

            if (!filter.PassFilter(display.Str())) {
                continue;
            }

            bool is_selected = (ps->SelectedEntityID == entity.ID);
            if (ImGui::Selectable(display.Str(), is_selected)) {
                SDL_Log("Selected entity %d", entity.ID.Value);
                ps->SelectedEntityID = entity.ID;
            }

            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndListBox();
    }
}

template <typename T>
constexpr bool HasBuildImGuiV = requires(T* ptr) { ::kdk::BuildImGui(ptr); };

template <typename T>
void BuildComponentImGui(T* component) {
    auto scratch = GetScratchArena();

    if constexpr (HasBuildImGuiV<T>) {
        String label = Printf(scratch.Arena,
                              "%s (Index: %d)",
                              ToString(T::kComponentType),
                              component->GetComponentIndex());
        if (ImGui::TreeNodeEx(label.Str(), ImGuiTreeNodeFlags_Framed)) {
            BuildImGui(component);
            ImGui::TreePop();
        }
    } else {
        String msg = Printf(scratch.Arena, "%s: No ImGui support", ToString(T::kComponentType));
        ImGui::Text("%s", msg.Str());
    }
}

void BuildImGui(EntityManager* eem, EntityID id) {
    if (!IsValid(*eem, id)) {
        ImGui::Text("Entity %d: Not valid", id.Value);
        return;
    }

    Entity* entity = GetEntity(eem, id);
    ASSERT(entity);

    ImGui::Text("ID: %d (Index: %d, Gen: %d) - Type: %s\n",
                id.Value,
                id.GetIndex(),
                id.GetGeneration(),
                ToString(entity->EntityType));
    auto* signature = GetEntitySignature(eem, id);
    ASSERT(signature);
    BuildImGui_EntitySignature(*signature);

    BuildImGui(&entity->Transform);

#define X(component_enum_name, component_type, ...)                                \
    case EEntityComponentType::component_enum_name: {                              \
        auto [component_index, component] = GetComponent<component_type>(eem, id); \
        if (component_index != NONE) {                                             \
            BuildComponentImGui(component);                                        \
        }                                                                          \
        break;                                                                     \
    }

    for (u8 i = 0; i < (u8)EEntityComponentType::COUNT; i++) {
        EEntityComponentType component_type = (EEntityComponentType)i;

        switch (component_type) {
            ECS_COMPONENT_TYPES(X)
            default: ASSERTF(false, "Unknown component type %d", (u8)component_type); return;
        }
    }
#undef X
}

template <typename T>
constexpr bool HasBuildGizmosV =
    requires(PlatformState* ps, T* ptr) { ::kdk::BuildGizmos(ps, ptr); };

template <typename T>
void BuildComponentGizmos(PlatformState* ps, T* component) {
    if constexpr (HasBuildGizmosV<T>) {
        BuildGizmos(ps, component);
    }
}

void BuildGizmos(PlatformState* ps, const Camera& camera, EntityManager* eem, EntityID id) {
    if (!IsValid(*eem, id)) {
        return;
    }

    Entity* entity = GetEntity(eem, id);
    ASSERT(entity);

    // Transform gizmo.
    {
        Mat4 model(1.0f);
        model = Translate(model, Vec3(entity->Transform.Position));
        if (ImGuizmo::Manipulate(GetPtr(camera.M_View),
                                 GetPtr(camera.M_Proj),
                                 ImGuizmo::TRANSLATE,
                                 ImGuizmo::WORLD,
                                 GetPtr(model))) {
            entity->Transform.Position = model[3];
        }
    }

#define X(component_enum_name, component_type, ...)                                \
    case EEntityComponentType::component_enum_name: {                              \
        auto [component_index, component] = GetComponent<component_type>(eem, id); \
        if (component_index != NONE) {                                             \
            BuildComponentGizmos(ps, component);                                   \
        }                                                                          \
        break;                                                                     \
    }

    for (u8 i = 0; i < (u8)EEntityComponentType::COUNT; i++) {
        EEntityComponentType component_type = (EEntityComponentType)i;

        switch (component_type) {
            ECS_COMPONENT_TYPES(X)
            default: ASSERTF(false, "Unknown component type %d", (u8)component_type); return;
        }
    }
#undef X
}

// TEMPLATE IMPLEMENTATION
// -------------------------------------------------------------------------

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
        ComponentToEntity[i] = {i + 1};
    }
    ComponentToEntity.back() = {};

    ASSERT(Owner == nullptr);
    Owner = entity_manager;

    SDL_Log("Initialized ComponentHolder: %s\n", ToString(component_type));
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
    NextComponent = ComponentToEntity[component_index].Value;
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
    component->_EntityManager = Owner;
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
    ASSERT(component->_EntityManager == Owner);
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

// TEST COMPONENTS ---------------------------------------------------------------------------------

void Serialize(SerdeArchive* sa, TestComponent* tc) { SERDE(sa, tc, Value); }

void Serialize(SerdeArchive* sa, Test2Component* tc) {
    SERDE(sa, tc, Name);
    SERDE(sa, tc, Transform);
}

}  // namespace kdk

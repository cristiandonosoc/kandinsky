#pragma once

#include <kandinsky/camera.h>
#include <kandinsky/core/container.h>
#include <kandinsky/core/defines.h>
#include <kandinsky/core/math.h>

#include <array>

namespace kdk {

struct EntityManager;
struct SerdeArchive;

EntityManager* GetRunningEntityManager();

// COMPONENT DEFINTIIONS ---------------------------------------------------------------------------

static constexpr i32 kMaxComponentTypes = 31;

// X macro for defining component types.
// Format: (component_enum_name, component_struct_name, component_max_count)
#define ECS_COMPONENT_TYPES(X)                         \
    X(StaticModel, StaticModelComponent, 128)          \
    X(PointLight, PointLightComponent, 16)             \
    X(DirectionalLight, DirectionalLightComponent, 16) \
    X(Spotlight, SpotlightComponent, 16)               \
    X(Test, TestComponent, kMaxEntities)               \
    X(Test2, Test2Component, kMaxEntities)

// Create the component enum.
enum class EEntityComponentType : u8 {
#define X(enum_name, ...) enum_name,
    ECS_COMPONENT_TYPES(X)
#undef X
    COUNT
};
static_assert((i32)EEntityComponentType::COUNT < kMaxComponentTypes,
              "Too many component types defined!");

// TODO(cdc): Right now we embed the EntityManager, the Owner entity and the component index
//			  direcly into the component.
//
//            In the future it could be calculated from the component pointer itself (doing an
//            address diff against the beginning of the owner array).
//            Also if we can have a global reference to the owning EntityManager, we could get away
//            with having NO backpointer data and still get all the benefit.
#define GENERATE_COMPONENT(component_name)                                                       \
    static constexpr const char* kComponentName = #component_name;                               \
    static constexpr EEntityComponentType kComponentType = EEntityComponentType::component_name; \
    ::kdk::EntityID _OwnerID = {};                                                               \
    ::kdk::EntityComponentIndex _ComponentIndex = NONE;                                          \
    ::kdk::EntityID GetOwnerID() const { return _OwnerID; }                                      \
    ::kdk::EntityComponentIndex GetComponentIndex() const { return _ComponentIndex; }            \
    ::kdk::Entity* GetOwner() { return ::kdk::GetEntity(GetRunningEntityManager(), _OwnerID); }  \
    const ::kdk::Entity* GetOwner() const {                                                      \
        return ::kdk::GetEntity(*GetRunningEntityManager(), _OwnerID);                           \
    }

// DECLARATIONS ------------------------------------------------------------------------------------

struct EntityID {
    // 8-bit generation, 24-bit index.
    i32 Value = NONE;

    bool operator==(i32 value) const { return Value == value; }
    bool operator==(const EntityID& other) const { return Value == other.Value; }

    i32 GetIndex() const { return Value & 0xFFFFFF; }
    u8 GetGeneration() const { return (u8)(Value >> 24); }

    static EntityID Build(i32 index, u8 generation) {
        return EntityID{generation << 24 | (index & 0xFFFFFF)};
    }
};

using EntityComponentIndex = i32;
using EntitySignature = i32;

static constexpr i32 kMaxEntities = 4096;
static constexpr i32 kNewEntitySignature = 1 << kMaxComponentTypes;  // Just the first bit set.

enum class EEntityType : u8 {
    Invalid = 0,
    Player,
    Enemy,
    NPC,
    Item,
    Projectile,
    PointLight,
    DirectionalLight,
    Spotlight,
    COUNT,
};
const char* ToString(EEntityType entity_type);

struct Entity {
    EntityID ID = {};
    EEntityType EntityType = EEntityType::Invalid;
    FixedString<128> Name = {};
    Transform Transform = {};
    Mat4 M_Model = {};

    // Used only for serialization, use |GetEntitySignature| instead.
    EntitySignature _Signature = NONE;
};

// ENTITY MANAGER ----------------------------------------------------------------------------------

struct EntityComponentSet;  // Forward declare.

inline bool IsLive(const EntitySignature& signature) { return signature < 0; }
bool ContainsComponent(EntityID id, EEntityComponentType component_type);
bool Matches(const EntitySignature& signature, EEntityComponentType component_type);

const char* ToString(EEntityComponentType component_type);

struct CreateEntityOptions {
    EEntityType EntityType = EEntityType::Invalid;
    String Name = {};
    Transform Transform = {};

    // ADVANCED OPTIONS!
    // Normally these are used by the serde system, use carefully.
    EntityID _Advanced_OverrideID = {};  // Normally you want to use the one given by the system.
};
std::pair<EntityID, Entity*> CreateEntity(EntityManager* em,
                                          const CreateEntityOptions& options = {});
void DestroyEntity(EntityManager* em, EntityID id);

bool IsValid(const EntityManager& em, EntityID id);
EntitySignature* GetEntitySignature(EntityManager* em, EntityID id);
Entity* GetEntity(EntityManager* em, EntityID id);
const Entity* GetEntity(const EntityManager& em, EntityID id);

void VisitEntities(EntityManager* em, const kdk::Function<bool(EntityID, Entity*)>& visitor);

void UpdateModelMatrices(EntityManager* em);

void Serialize(SerdeArchive* sa, EntityManager* em);
void Serialize(SerdeArchive* sa, Entity* entity);

// COMPONENT MANAGEMENT ----------------------------------------------------------------------------
//
// The template variations you can pass the type of the component and most results will be
// specialized to that type. There are opaque calls beneath for code that has to be type agnostic.
//
// NOTE: Some of these functions are defined in entity_manager.cpp, for linking purposes with the
//       component holders templates.

template <typename T>
std::pair<EntityComponentIndex, T*> AddComponent(EntityManager* em,
                                                 EntityID id,
                                                 const T* initial_values = nullptr);
std::pair<EntityComponentIndex, void*> AddComponent(EntityManager* em,
                                                    EntityID id,
                                                    EEntityComponentType component_type,
                                                    const void* initial_values = nullptr);

template <typename T>
std::pair<EntityComponentIndex, T*> GetComponent(EntityManager* em, EntityID id);
EntityComponentIndex GetComponent(EntityManager* em,
                                  EntityID id,
                                  EEntityComponentType component_type,
                                  void** out);

template <typename T>
bool RemoveComponent(EntityManager* em, EntityID id);
bool RemoveComponent(EntityManager* em, EntityID id, EEntityComponentType component_type);

template <typename T>
i32 GetComponentCount(const EntityManager& em);
i32 GetComponentCount(const EntityManager& em, EEntityComponentType component_type);

template <typename T>
std::span<EntityID> GetEntitiesWithComponent(EntityManager* em);
std::span<EntityID> GetEntitiesWithComponent(EntityManager* em,
                                             EEntityComponentType component_type);
// The visitor returns whether you want to continue iterating or not (return false to stop).
template <typename T>
void VisitComponents(EntityManager* em, const kdk::Function<bool(EntityID, Entity*, T*)>& visitor);

template <typename T>
EntityComponentIndex GetComponentIndex(const EntityManager& em, EntityID id);
EntityComponentIndex GetComponentIndex(const EntityManager& em,
                                       EntityID id,
                                       EEntityComponentType component_type);

template <typename T>
EntityID GetOwningEntity(const EntityManager& em, EntityComponentIndex component_index);
EntityID GetOwningEntity(const EntityManager& em,
                         EEntityComponentType component_type,
                         EntityComponentIndex component_index);
// IMGUI -------------------------------------------------------------------------------------------

void BuildEntityListImGui(PlatformState* ps, EntityManager* em);
void BuildEntityDebuggerImGui(PlatformState* ps, EntityManager* em);

void BuildImGui(EntityManager* em, EntityID id);
void BuildGizmos(PlatformState* ps, const Camera& camera, EntityManager* em, EntityID id);

// TEST COMPONENTS ---------------------------------------------------------------------------------

struct TestComponent {
    GENERATE_COMPONENT(Test);

    i32 Value = 0;
};
void Serialize(SerdeArchive*, TestComponent*);

struct Test2Component {
    GENERATE_COMPONENT(Test2);

    String Name = {};
    Transform Transform = {};
};
void Serialize(SerdeArchive*, Test2Component*);

// TEMPLATE IMPLEMENTATION -------------------------------------------------------------------------

template <typename T>
std::pair<EntityComponentIndex, T*> GetComponent(EntityManager* em, EntityID id) {
    T* out = nullptr;
    EntityComponentIndex component_index = GetComponent(em, id, T::kComponentType, (void**)&out);
    return {component_index, out};
}

template <typename T>
std::pair<EntityComponentIndex, T*> AddComponent(EntityManager* em,
                                                 EntityID id,
                                                 const T* initial_values) {
    auto [component_index, component] = AddComponent(em, id, T::kComponentType, initial_values);
    return {component_index, (T*)component};
}

template <typename T>
bool RemoveComponent(EntityManager* em, EntityID id) {
    return RemoveComponent(em, id, T::kComponentType);
}

template <typename T>
i32 GetComponentCount(const EntityManager& em) {
    return GetComponentCount(em, T::kComponentType);
}

template <typename T>
std::span<EntityID> GetEntitiesWithComponent(EntityManager* em) {
    return GetEntitiesWithComponent(em, T::kComponentType);
}

template <typename T>
EntityComponentIndex GetComponentIndex(const EntityManager& em, EntityID id) {
    return GetComponentIndex(em, id, T::kComponentType);
}

template <typename T>
EntityID GetOwningEntity(const EntityManager& em, EntityComponentIndex component_index) {
    return GetOwningEntity(em, T::kComponentType, component_index);
}

template <typename T>
void VisitComponents(EntityManager* em, const kdk::Function<bool(EntityID, Entity*, T*)>& visitor) {
    auto entities = GetEntitiesWithComponent(em, T::kComponentType);
    for (EntityID id : entities) {
        auto* entity = GetEntity(em, id);

        auto [component_index, component] = GetComponent<T>(em, id);
        ASSERT(component_index != NONE);

        if (!visitor(id, entity, component)) [[unlikely]] {
            continue;
        }
    }
}

}  // namespace kdk

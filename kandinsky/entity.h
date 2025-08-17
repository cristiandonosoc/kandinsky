#pragma once

#include <kandinsky/camera.h>
#include <kandinsky/core/container.h>
#include <kandinsky/core/defines.h>
#include <kandinsky/core/math.h>

#include <array>

namespace kdk {

struct SerdeArchive;

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
    X(Test2, TestComponent, kMaxEntities)

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
    ::kdk::EntityManager* _EntityManager = nullptr;                                              \
    ::kdk::EntityID _OwnerID = {};                                                               \
    ::kdk::EntityComponentIndex _ComponentIndex = NONE;                                          \
    ::kdk::EntityID GetOwnerID() const { return _OwnerID; }                                      \
    ::kdk::EntityComponentIndex GetComponentIndex() const { return _ComponentIndex; }            \
    ::kdk::Entity* GetOwner() { return ::kdk::GetEntity(_EntityManager, _OwnerID); }             \
    const ::kdk::Entity* GetOwner() const { return ::kdk::GetEntity(*_EntityManager, _OwnerID); }

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

struct EntityManager {
    i32 NextIndex = 0;
    i32 EntityCount = 0;
    std::array<u8, kMaxEntities> Generations = {};
    std::array<EntitySignature, kMaxEntities> Signatures = {};
    std::array<Entity, kMaxEntities> Entities = {};

    EntityComponentSet* Components = nullptr;
};

void Init(Arena* arena, EntityManager* eem);
void Shutdown(EntityManager* eem);

struct CreateEntityOptions {
    EEntityType EntityType = EEntityType::Invalid;
    String Name = {};
    Transform Transform = {};

    // ADVANCED OPTIONS!
    // Normally these are used by the serde system, use carefully.
    EntityID _Advanced_OverrideID;  // Normally you want to use the one given by the system.
};
std::pair<EntityID, Entity*> CreateEntity(EntityManager* eem,
                                          const CreateEntityOptions& options = {});
void DestroyEntity(EntityManager* eem, EntityID id);

bool IsValid(const EntityManager& eem, EntityID id);
EntitySignature* GetEntitySignature(EntityManager* eem, EntityID id);
Entity* GetEntity(EntityManager* eem, EntityID id);
const Entity* GetEntity(const EntityManager& eem, EntityID id);

void VisitEntities(EntityManager* eem, const kdk::Function<bool(EntityID, Entity*)>& visitor);

void UpdateModelMatrices(EntityManager* eem);

void Serialize(SerdeArchive* sa, EntityManager* eem);
void Serialize(SerdeArchive* sa, Entity* entity);

// COMPONENT MANAGEMENT ----------------------------------------------------------------------------

std::pair<EntityComponentIndex, void*> AddComponent(EntityManager* eem,
                                                    EntityID id,
                                                    EEntityComponentType component_type,
                                                    const void* initial_values = nullptr);

inline EntityComponentIndex AddComponentTest(EntityManager* eem,
                                             EntityID id,
                                             EEntityComponentType component_type) {
    auto [i, _] = AddComponent(eem, id, component_type, nullptr);
    return i;
}

template <typename T>
std::pair<EntityComponentIndex, T*> AddComponent(EntityManager* eem,
                                                 EntityID id,
                                                 const T* initial_values = nullptr) {
    auto [component_index, component] = AddComponent(eem, id, T::kComponentType, initial_values);
    return {component_index, (T*)component};
}

EntityComponentIndex GetComponent(EntityManager* eem,
                                  EntityID id,
                                  EEntityComponentType component_type,
                                  void** out);
template <typename T>
std::pair<EntityComponentIndex, T*> GetComponent(EntityManager* eem, EntityID id) {
    T* out = nullptr;
    EntityComponentIndex component_index = GetComponent(eem, id, T::kComponentType, (void**)&out);
    return {component_index, out};
}

bool RemoveComponent(EntityManager* eem, EntityID id, EEntityComponentType component_type);
template <typename T>
bool RemoveComponent(EntityManager* eem, EntityID id) {
    return RemoveComponent(eem, id, T::kComponentType);
}

i32 GetComponentCount(const EntityManager& eem, EEntityComponentType component_type);
template <typename T>
i32 GetComponentCount(const EntityManager& eem) {
    return GetComponentCount(eem, T::kComponentType);
}

std::span<EntityID> GetEntitiesWithComponent(EntityManager* eem,
                                             EEntityComponentType component_type);
template <typename T>
std::span<EntityID> GetEntitiesWithComponent(EntityManager* eem) {
    return GetEntitiesWithComponent(eem, T::kComponentType);
}

// The visitor returns whether you want to continue iterating or not (return false to stop).
template <typename T>
void VisitComponents(EntityManager* eem,
                     const kdk::Function<bool(EntityID, Entity*, T*)>& visitor) {
    auto entities = GetEntitiesWithComponent(eem, T::kComponentType);
    for (EntityID id : entities) {
        auto* entity = GetEntity(eem, id);

        auto [component_index, component] = GetComponent<T>(eem, id);
        ASSERT(component_index != NONE);

        if (!visitor(id, entity, component)) [[unlikely]] {
            continue;
        }
    }
}

EntityComponentIndex GetComponentIndex(const EntityManager& eem,
                                       EntityID id,
                                       EEntityComponentType component_type);
template <typename T>
EntityComponentIndex GetComponentIndex(const EntityManager& eem, EntityID id) {
    return GetComponentIndex(eem, id, T::kComponentType);
}

EntityID GetOwningEntity(const EntityManager& eem,
                         EEntityComponentType component_type,
                         EntityComponentIndex component_index);
template <typename T>
EntityID GetOwningEntity(const EntityManager& eem, EntityComponentIndex component_index) {
    return GetOwningEntity(eem, T::kComponentType, component_index);
}

void BuildEntityListImGui(PlatformState* ps, EntityManager* eem);

void BuildImGui(EntityManager* eem, EntityID id);
void BuildGizmos(PlatformState* ps, const Camera& camera, EntityManager* eem, EntityID id);

// Test components ---------------------------------------------------------------------------------

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

}  // namespace kdk

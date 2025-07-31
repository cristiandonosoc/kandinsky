#pragma once

#include <kandinsky/defines.h>
#include <kandinsky/math.h>

#include <array>

namespace kdk {

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
static constexpr i32 kMaxComponentTypes = 31;
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

struct Entity {
    EntityID ID = {};
    EEntityType EntityType = EEntityType::Invalid;
    Transform Transform = {};
    Mat4 M_Model = {};
};

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
    ::kdk::Entity* GetOwner() { return ::kdk::GetEntity(_EntityManager, _OwnerID); }     \
    const ::kdk::Entity* GetOwner() const {                                                  \
        return ::kdk::GetEntity(*_EntityManager, _OwnerID);                                  \
    }

// X macro for defining component types.
// Format: (component_enum_name, component_struct_name, component_max_count)
#define ECS_COMPONENT_TYPES(X)                         \
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

// ENTITY MANAGER ----------------------------------------------------------------------------------

struct EntityComponentSet;  // Forward declare.

inline bool IsLive(const EntitySignature& signature) { return signature < 0; }
bool ContainsComponent(EntityID id, EEntityComponentType component_type);
bool Matches(const EntitySignature& signature, EEntityComponentType component_type);

const char* ToString(EEntityComponentType component_type);

struct EntityManager {
    std::array<Entity, kMaxEntities> Entities = {};
    std::array<EntitySignature, kMaxEntities> Signatures = {};
    std::array<u8, kMaxEntities> Generations = {};
    i32 NextIndex = 0;
    i32 EntityCount = 0;

    EntityComponentSet* Components = nullptr;
};

void Init(Arena* arena, EntityManager* eem);
void Shutdown(EntityManager* eem);

EntityID CreateEntity(EntityManager* eem, Entity** out_data = nullptr);
void DestroyEntity(EntityManager* eem, EntityID id);

bool IsValid(const EntityManager& eem, EntityID id);
EntitySignature* GetEntitySignature(EntityManager* eem, EntityID id);
Entity* GetEntity(EntityManager* eem, EntityID id);
const Entity* GetEntity(const EntityManager& eem, EntityID id);

void UpdateModelMatrices(EntityManager* eem);

// COMPONENT MANAGEMENT ----------------------------------------------------------------------------

EntityComponentIndex AddComponent(EntityManager* eem,
                                  EntityID id,
                                  EEntityComponentType component_type,
                                  void** out = nullptr);
template <typename T>
std::pair<EntityComponentIndex, T*> AddComponent(EntityManager* eem, EntityID id) {
    T* out = nullptr;
    EntityComponentIndex component_index = AddComponent(eem, id, T::kComponentType, (void**)&out);
    return {component_index, out};
}

EntityComponentIndex GetComponent(EntityManager* eem,
                                  EntityID id,
                                  EEntityComponentType component_type,
                                  void** out);
template <typename T>
std::pair<EntityComponentIndex, T*> GetComponent(EntityManager* eem, EntityID id) {
    T* out = nullptr;
    EntityComponentIndex component_index = GetComponent(eem, id, T::kComponentType, &out);
    return {component_index, out};
}

bool RemoveComponent(EntityManager* eem, EntityID id, EEntityComponentType component_type);
template <typename T>
bool RemoveComponent(EntityManager* eem, EntityID id) {
    return RemoveComponent(eem, id, T::kComponentType);
}

void BuildImGui(EntityManager* eem, EntityID id);

i32 GetComponentCount(const EntityManager& eem, EEntityComponentType component_type);
template <typename T>
i32 GetComponentCount(const EntityManager& eem) {
    return GetComponentCount(eem, T::kComponentType);
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

// Test components ---------------------------------------------------------------------------------

struct TestComponent {
    GENERATE_COMPONENT(Test);

    i32 Value = 0;
};

struct Test2Component {
    GENERATE_COMPONENT(Test2);

    String Name = {};
    Transform Transform = {};
};

}  // namespace kdk

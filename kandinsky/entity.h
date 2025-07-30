#pragma once

#include <kandinsky/defines.h>
#include <kandinsky/math.h>

#include <array>

namespace kdk {

// DECLARATIONS ------------------------------------------------------------------------------------

using Entity = i32;  // 8-bit generation, 24-bit index.
inline i32 GetEntityIndex(Entity entity) { return entity & 0xFFFFFF; }
inline u8 GetEntityGeneration(Entity entity) { return (u8)(entity >> 24); }
inline Entity BuildEntity(i32 index, u8 generation) {
    return generation << 24 | (index & 0xFFFFFF);
}

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

struct EntityData {
    Entity EntityID = NONE;
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
    ::kdk::Entity _OwnerID = NONE;                                                               \
    ::kdk::EntityComponentIndex _ComponentIndex = NONE;                                          \
    ::kdk::Entity GetOwnerID() const { return _OwnerID; }                                        \
    ::kdk::Entity GetComponentIndex() const { return _ComponentIndex; }                          \
    ::kdk::EntityData* GetOwner() { return ::kdk::GetEntityData(_EntityManager, _OwnerID); }     \
    const ::kdk::EntityData* GetOwner() const {                                                  \
        return ::kdk::GetEntityData(*_EntityManager, _OwnerID);                                  \
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
bool ContainsComponent(Entity entity, EEntityComponentType component_type);
bool Matches(const EntitySignature& signature, EEntityComponentType component_type);

const char* ToString(EEntityComponentType component_type);

struct EntityManager {
    std::array<EntityData, kMaxEntities> EntityDatas = {};
    std::array<EntitySignature, kMaxEntities> Signatures = {};
    std::array<u8, kMaxEntities> Generations = {};
    i32 NextIndex = 0;
    i32 EntityCount = 0;

    EntityComponentSet* Components = nullptr;
};

void Init(Arena* arena, EntityManager* eem);
void Shutdown(EntityManager* eem);

Entity CreateEntity(EntityManager* eem, EntityData** out_data = nullptr);
void DestroyEntity(EntityManager* eem, Entity entity);

bool IsValid(const EntityManager& eem, Entity entity);
EntitySignature* GetEntitySignature(EntityManager* eem, Entity entity);
EntityData* GetEntityData(EntityManager* eem, Entity entity);
const EntityData* GetEntityData(const EntityManager& eem, Entity entity);

void UpdateModelMatrices(EntityManager* eem);

// COMPONENT MANAGEMENT ----------------------------------------------------------------------------

EntityComponentIndex AddComponent(EntityManager* eem,
                                  Entity entity,
                                  EEntityComponentType component_type,
                                  void** out = nullptr);
template <typename T>
std::pair<EntityComponentIndex, T*> AddComponent(EntityManager* eem, Entity entity) {
    T* out = nullptr;
    EntityComponentIndex component_index =
        AddComponent(eem, entity, T::kComponentType, (void**)&out);
    return {component_index, out};
}

EntityComponentIndex GetComponent(EntityManager* eem,
                                  Entity entity,
                                  EEntityComponentType component_type,
                                  void** out);
template <typename T>
std::pair<EntityComponentIndex, T*> GetComponent(EntityManager* eem, Entity entity) {
    T* out = nullptr;
    EntityComponentIndex component_index = GetComponent(eem, entity, T::kComponentType, &out);
    return {component_index, out};
}

bool RemoveComponent(EntityManager* eem, Entity entity, EEntityComponentType component_type);
template <typename T>
bool RemoveComponent(EntityManager* eem, Entity entity) {
    return RemoveComponent(eem, entity, T::kComponentType);
}

void BuildImGui(EntityManager* eem, Entity entity);

i32 GetComponentCount(const EntityManager& eem, EEntityComponentType component_type);
template <typename T>
i32 GetComponentCount(const EntityManager& eem) {
    return GetComponentCount(eem, T::kComponentType);
}

EntityComponentIndex GetComponentIndex(const EntityManager& eem,
                                       Entity entity,
                                       EEntityComponentType component_type);
template <typename T>
EntityComponentIndex GetComponentIndex(const EntityManager& eem, Entity entity) {
    return GetComponentIndex(eem, entity, T::kComponentType);
}

Entity GetOwningEntity(const EntityManager& eem,
                       EEntityComponentType component_type,
                       EntityComponentIndex component_index);
template <typename T>
Entity GetOwningEntity(const EntityManager& eem, EntityComponentIndex component_index) {
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

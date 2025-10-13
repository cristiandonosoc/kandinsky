#pragma once

#include <kandinsky/camera.h>
#include <kandinsky/core/container.h>
#include <kandinsky/core/defines.h>
#include <kandinsky/core/math.h>

namespace kdk {

struct EntityManager;
struct SerdeArchive;
struct Scene;

using EntityComponentIndex = i32;
using EntitySignature = i32;

static constexpr i32 kMaxEntities = 16 * 1024;  // 2^14
static constexpr i32 kMaxComponentTypes = 31;
static constexpr i32 kNewEntitySignature = 1 << kMaxComponentTypes;  // Just the first bit set.

EntityManager* GetRunningEntityManager();

// COMPONENT DEFINTIIONS ---------------------------------------------------------------------------

// Definition of entities. Total count must be less than kMaxEntities.
// Format: (ENUM_NAME, STRUCT_NAME, MAX_COUNT)

#define ENTITY_TYPES(X)                   \
    X(Player, PlayerEntity, 4)            \
    X(Spawner, SpawnerEntity, 256)        \
    X(Enemy, EnemyEntity, 4096)           \
    X(Building, BuildingEntity, 256)      \
    X(Projectile, ProjectileEntity, 1024) \
    X(Test, TestEntity, 1024)

// Create the enum.
enum class EEntityType : u8 {
    Invalid = 0,
#define X(ENUM_NAME, ...) ENUM_NAME,
    ENTITY_TYPES(X)
#undef X
    COUNT,
};
String ToString(EEntityType entity_type);

// Forward declare.
#define X(ENUM_NAME, STRUCT_NAME, ...) struct STRUCT_NAME;
ENTITY_TYPES(X)
#undef X

#define GENERATE_ENTITY(ENUM_NAME)                                                                \
    static_assert((i32)EEntityType::ENUM_NAME < (i32)EEntityType::COUNT, "Invalid entity type!"); \
    static constexpr String kEntityName{std::string_view(#ENUM_NAME)};                            \
    static constexpr EEntityType kEntityType = EEntityType::ENUM_NAME;                            \
    ::kdk::Entity* _Entity = {};                                                                  \
    ::kdk::EntityID GetEntityID() const { return _Entity->ID; }                                   \
    ::kdk::Entity* GetEntity() { return _Entity; }                                                \
    const ::kdk::Entity* GetEntity() const { return _Entity; }                                    \
    const Mat4& GetModelMatrix() const {                                                          \
        return ::kdk::GetModelMatrix(*GetRunningEntityManager(), _Entity->ID);                    \
    }                                                                                             \
    const auto& GetName() const { return _Entity->Name; }                                         \
    Transform& GetTransform() { return _Entity->Transform; }                                      \
    const Transform& GetTransform() const { return _Entity->Transform; }

// X macro for defining component types.
// Format: (component_enum_name, component_struct_name, component_max_count)
#define COMPONENT_TYPES(X)                             \
    X(StaticModel, StaticModelComponent, 128)          \
    X(PointLight, PointLightComponent, 16)             \
    X(DirectionalLight, DirectionalLightComponent, 16) \
    X(Spotlight, SpotlightComponent, 16)               \
    X(Health, HealthComponent, kMaxEntities)           \
    X(Billboard, BillboardComponent, kMaxEntities)     \
    X(Test, TestComponent, kMaxEntities)               \
    X(Test2, Test2Component, kMaxEntities)

// Create the component enum.
enum class EEntityComponentType : u8 {
#define X(ENUM_NAME, ...) ENUM_NAME,
    COMPONENT_TYPES(X)
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
    // 8-bit entity type, 8-bit generation, 24-bit index.
    union {
        i32 RawValue = NONE;
        struct {
            i16 Index;
            u8 Generation;
            EEntityType EntityType;
        } Values;
    };

    bool operator==(i32 raw) const { return RawValue == raw; }
    bool operator==(const EntityID& other) const { return RawValue == other.RawValue; }
    bool operator<(const EntityID& other) const { return RawValue < other.RawValue; }
    bool operator<=(const EntityID& other) const { return RawValue <= other.RawValue; }

    i32 GetIndex() const { return Values.Index; }
    u8 GetGeneration() const { return Values.Generation; }
    EEntityType GetEntityType() const { return Values.EntityType; }

    static EntityID Build(i16 index, u8 generation, EEntityType entity_type) {
        return EntityID{
            .Values = {.Index = index, .Generation = generation, .EntityType = entity_type}
        };
    }
};
inline bool IsNone(EntityID id) { return id.RawValue == NONE; }
bool IsValid(const EntityManager& em, EntityID id);

const Mat4& GetModelMatrix(const EntityManager& em, EntityID id);

struct EntityFlags {
    bool OnGrid : 1 = false;  // Is this entity snapped to the grid?
};
static_assert(sizeof(EntityFlags) == 1);

struct Entity {
    EntityID ID = {};
    EntityFlags Flags = {};
    // TODO(cdc): Move this to a separate array.
    FixedString<128> Name = {};
    Transform Transform = {};

    // Used only for serialization, use |GetEntitySignature| instead.
    EntitySignature _Signature = NONE;

    EEntityType GetEntityType() const { return ID.GetEntityType(); }
    const Mat4& GetModelMatrix() const {
        return ::kdk::GetModelMatrix(*::kdk::GetRunningEntityManager(), ID);
    }
};

inline bool IsLive(const EntitySignature& signature) { return signature < 0; }
bool ContainsComponent(EntityID id, EEntityComponentType component_type);
bool Matches(const EntitySignature& signature, EEntityComponentType component_type);

IVec2 GetGridCoord(const Entity& entity);

String ToString(EEntityComponentType component_type);

struct CreateEntityOptions {
    String Name = {};
    Transform Transform = {};
    EntityFlags Flags = {};

    // ADVANCED OPTIONS!
    // Normally these are used by the serde system, use carefully.
    EntityID _Advanced_OverrideID = {};  // Normally you want to use the one given by the system.
};
std::pair<EntityID, Entity*> CreateEntityOpaque(EntityManager* em,
                                                EEntityType entity_type,
                                                const CreateEntityOptions& options = {},
                                                const void* initial_values = nullptr);

template <typename T>
std::pair<EntityID, T*> CreateEntity(EntityManager* em,
                                     const CreateEntityOptions& options = {},
                                     const T* initial_values = nullptr) {
    auto [id, entity] = CreateEntityOpaque(em, T::kEntityType, options, initial_values);
    auto* typed = GetTypedEntity<T>(em, id);
    return {id, typed};
}

void DestroyEntity(EntityManager* em, EntityID id);

std::pair<EntityID, Entity*> CloneEntity(EntityManager* em, EntityID id);

void* GetTypedEntityOpaque(EntityManager* em, EntityID id);

template <typename T>
T* GetTypedEntity(EntityManager* em, EntityID id) {
    ASSERT(id.GetEntityType() == T::kEntityType);
    return (T*)GetTypedEntityOpaque(em, id);
}

EntitySignature* GetEntitySignature(EntityManager* em, EntityID id);
const EntitySignature* GetEntitySignature(const EntityManager& em, EntityID id);
Entity* GetEntity(EntityManager* em, EntityID id);
const Entity* GetEntity(const EntityManager& em, EntityID id);

void VisitAllEntities(EntityManager* em, const kdk::Function<bool(EntityID, Entity*)>& visitor);
void VisitAllEntities(const EntityManager& em,
                      const kdk::Function<bool(EntityID, const Entity&)>& visitor);

void VisitEntitiesOpaque(EntityManager* em,
                         EEntityType entity_type,
                         const kdk::Function<bool(EntityID, void*)>& visitor);

template <typename T>
void VisitEntities(EntityManager* em, const kdk::Function<bool(EntityID, T*)>& visitor) {
    VisitEntitiesOpaque(em, T::kEntityType, [&visitor](EntityID id, void* elem) {
        return visitor(id, (T*)elem);
    });
}

void Serialize(SerdeArchive* sa, Entity* entity);
void Serialize(SerdeArchive* sa, EntityFlags* flags);

// COMPONENT MANAGEMENT ----------------------------------------------------------------------------
//
// The template variations you can pass the type of the component and most results will be
// specialized to that type. There are opaque calls beneath for code that has to be type agnostic.
//
// NOTE: Some of these functions are defined in entity_manager.cpp, for linking purposes with the
//       component holders templates.

template <typename T>
bool HasComponent(const EntityManager& em, EntityID id);
bool HasComponent(const EntityManager& em, EntityID id, EEntityComponentType component_type);

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

// VALIDATION --------------------------------------------------------------------------------------

struct ValidationError {
    FixedString<256> Message = {};
    Vec3 Position = {};
    EntityID EntityID = {};
};

bool ValidateEntity(Scene* scene, const Entity& entity, FixedVector<ValidationError, 64>* out);
bool IsValidPosition(Scene* scene, const Entity& entity);

// IMGUI -------------------------------------------------------------------------------------------

void BuildEntityListImGui(PlatformState* ps, EntityManager* em);
void BuildEntityDebuggerImGui(PlatformState* ps, EntityManager* em);

void BuildImGui(PlatformState* ps, EntityManager* em, EntityID id);
void BuildGizmos(PlatformState* ps, const Camera& camera, EntityManager* em, EntityID id);

// TEST COMPONENTS ---------------------------------------------------------------------------------

struct TestEntity {
    GENERATE_ENTITY(Test);
};
inline void Validate(const Scene*, const TestEntity&, FixedVector<ValidationError, 64>*) {}
inline void Serialize(SerdeArchive*, TestEntity*) {}

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
bool HasComponent(EntityManager* em, EntityID id) {
    return HasComponent(*em, id, T::kComponentType);
}

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

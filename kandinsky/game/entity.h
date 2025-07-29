#pragma once

#include <kandinsky/defines.h>
#include <kandinsky/game/components.h>
#include <kandinsky/game/entity_definitions.h>
#include <kandinsky/math.h>

#include <kandinsky/graphics/light.h>

#include <array>
#include <variant>

namespace kdk {

inline bool IsLive(const EntitySignature& signature) { return signature < 0; }
bool ContainsComponent(Entity entity, EEntityComponentType component_type);
bool Matches(const EntitySignature& signature, EEntityComponentType component_type);

static_assert((i32)EEntityComponentType::COUNT < kMaxComponentTypes);

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
    COUNT
};

struct EntityData {
    Transform Transform = {};
    EEntityType EntityType = EEntityType::Invalid;
};

struct EntityManager {
    std::array<EntityData, kMaxEntities> EntityDatas = {};
    std::array<EntitySignature, kMaxEntities> Signatures = {};
    std::array<u8, kMaxEntities> Generations = {};
    i32 NextIndex = 0;
    i32 EntityCount = 0;

    // Create the component arrays.
#define X(component_enum_name, component_struct_name, component_max_count, ...) \
    EntityComponentHolder<component_struct_name, component_max_count>           \
        component_enum_name##ComponentHolder;

    ECS_COMPONENT_TYPES(X)
#undef X
};

void Init(EntityManager* eem);
void Shutdown(EntityManager* eem);

Entity CreateEntity(EntityManager* eem, EntityData** out_data = nullptr);
void DestroyEntity(EntityManager* eem, Entity entity);

bool IsValid(const EntityManager& eem, Entity entity);
EntitySignature* GetEntitySignature(EntityManager* eem, Entity entity);
EntityData* GetEntityData(EntityManager* eem, Entity entity);

bool AddComponent(EntityManager* eem, Entity entity, EEntityComponentType component_type);
bool RemoveComponent(EntityManager* eem, Entity entity, EEntityComponentType component_type);

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
EntityComponentIndex GetOwningEntity(const EntityManager& eem, Entity entity) {
    return GetOwningEntity(eem, entity, T::kComponentType);
}

}  // namespace kdk

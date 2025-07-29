#pragma once

#include <kandinsky/defines.h>
#include <kandinsky/ecs/ecs_components.h>
#include <kandinsky/ecs/ecs_definitions.h>
#include <kandinsky/math.h>

#include <array>

namespace kdk {

inline bool IsLive(const ECSEntitySignature& signature) { return signature < 0; }
bool ContainsComponent(ECSEntity entity, EECSComponentType component_type);
bool Matches(const ECSEntitySignature& signature, EECSComponentType component_type);

static_assert((i32)EECSComponentType::COUNT < kMaxComponentTypes);

enum class EEntityType : u8 {
    Invalid = 0,
    Player,
    Enemy,
    NPC,
    Item,
    Projectile,
    COUNT
};

struct EntityData {
    Transform Transform = {};
    EEntityType Type = EEntityType::Invalid;
};

struct ECSEntityManager {
    std::array<EntityData, kMaxEntities> EntityDatas = {};
    std::array<ECSEntitySignature, kMaxEntities> Signatures = {};
    std::array<u8, kMaxEntities> Generations = {};
    i32 NextIndex = 0;
    i32 EntityCount = 0;

    // Create the component arrays.
#define X(component_enum_name, component_struct_name, component_max_count, ...) \
    ECSComponentHolder<component_struct_name, component_max_count>              \
        component_enum_name##ComponentHolder;

    ECS_COMPONENT_TYPES(X)
#undef X
};

void Init(ECSEntityManager* eem);
void Shutdown(ECSEntityManager* eem);

ECSEntity CreateEntity(ECSEntityManager* eem, EntityData** out_data = nullptr);
void DestroyEntity(ECSEntityManager* eem, ECSEntity entity);

bool IsValid(const ECSEntityManager& eem, ECSEntity entity);
ECSEntitySignature* GetEntitySignature(ECSEntityManager* eem, ECSEntity entity);
EntityData* GetEntityData(ECSEntityManager* eem, ECSEntity entity);

bool AddComponent(ECSEntityManager* eem, ECSEntity entity, EECSComponentType component_type);
bool RemoveComponent(ECSEntityManager* eem, ECSEntity entity, EECSComponentType component_type);

i32 GetComponentCount(const ECSEntityManager& eem, EECSComponentType component_type);
template <typename T>
i32 GetComponentCount(const ECSEntityManager& eem) {
    return GetComponentCount(eem, T::kComponentType);
}

ECSComponentIndex GetComponentIndex(const ECSEntityManager& eem,
                                    ECSEntity entity,
                                    EECSComponentType component_type);
template <typename T>
ECSComponentIndex GetComponentIndex(const ECSEntityManager& eem, ECSEntity entity) {
    return GetComponentIndex(eem, entity, T::kComponentType);
}

ECSEntity GetOwningEntity(const ECSEntityManager& eem,
                          EECSComponentType component_type,
                          ECSComponentIndex component_index);
template <typename T>
ECSComponentIndex GetOwningEntity(const ECSEntityManager& eem, ECSEntity entity) {
    return GetOwningEntity(eem, entity, T::kComponentType);
}

}  // namespace kdk

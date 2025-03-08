#pragma once

#include <kandinsky/defines.h>
#include <kandinsky/math.h>

#include <array>

namespace kdk {

enum class EEntityType : u8 {
    Invalid = 0,
    Box,
    Model,
    DirectionalLight,
    PointLight,
    Spotlight,
};

struct Entity {
    u32 ID = 0;
    EEntityType Type = EEntityType::Invalid;
    void* Data = nullptr;
};
static_assert(sizeof(Entity) == 16);

inline bool IsValid(const Entity& e) { return e.Type != EEntityType::Invalid; }

struct EntityManager {
    static constexpr u32 kMaxEntityCount = 128;

    std::array<Entity, kMaxEntityCount> Entities = {};
    std::array<Transform, kMaxEntityCount> Transforms = {};
	std::array<Mat4, kMaxEntityCount> ModelMatrices = {};

    u32 EntityCount = 0;
    u32 HoverEntityID = 0;
	u32 SelectedEntityID = 0;
};

Entity* AddEntity(EntityManager* em,
                  EEntityType type,
                  const Transform& transform = {});
Entity* GetSelectedEntity(EntityManager& em);

Transform& GetEntityTransform(EntityManager* em, const Entity& entity);
const Mat4& GetEntityModelMatrix(EntityManager* em, const Entity& entity);



} // namespace kdk

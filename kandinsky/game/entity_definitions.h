#pragma once

#include <kandinsky/defines.h>

namespace kdk {

enum class EEntityComponentType : u8;  // Forward declare.

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
                                                                     //
#define GENERATE_COMPONENT(component_name)                         \
    static constexpr const char* kComponentName = #component_name; \
    static constexpr EEntityComponentType kComponentType = EEntityComponentType::component_name;

// X macro for defining component types.
// Format: (component_enum_name, component_struct_name, component_max_count)
#define ECS_COMPONENT_TYPES(X)                         \
    X(PointLight, PointLightComponent, 16)             \
    X(DirectionalLight, DirectionalLightComponent, 16) \
    X(Spotlight, SpotlightLightComponent, 16) \
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


}  // namespace kdk

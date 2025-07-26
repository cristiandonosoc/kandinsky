#include <kandinsky/ecs/ecs_components.h>

namespace kdk {

const char* ToString(EECSComponentType component_type) {
    // X-macro to find the component holder.
#define X(component_enum_name, ...) \
    case EECSComponentType::component_enum_name: return #component_enum_name;

    switch (component_type) {
        ECS_COMPONENT_TYPES(X)
        default:
            ASSERTF(false, "Unknown component type %d", (u8)component_type);
            return "<invalid>";
    }
#undef X
}

}  // namespace kdk

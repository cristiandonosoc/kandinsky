#include <kandinsky/entity.h>

#include <kandinsky/camera.h>
#include <kandinsky/graphics/light.h>
#include <kandinsky/graphics/opengl.h>
#include <kandinsky/imgui.h>
#include <kandinsky/memory.h>

namespace kdk {

const char* ToString(EEntityType entity_type) {
    switch (entity_type) {
        case EEntityType::Invalid: return "<INVALID>";
        case EEntityType::Box: return "Box";
        case EEntityType::DirectionalLight: return "DirectionalLight";
        case EEntityType::PointLight: return "PointLight";
        case EEntityType::Spotlight: return "Spotlight";
        case EEntityType::COUNT: return "<COUNT>";
    }

    ASSERT(false);
    return "<UNKNOWN>";
}

void BuildImgui(const EntityID& id) {
    ImGui::Text("%s - INDEX: %u (RAW: %u)", ToString(id.GetEntityType()), id.GetIndex(), id.ID);
}

}  // namespace kdk

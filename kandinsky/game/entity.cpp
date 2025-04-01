#include <kandinsky/game/entity.h>

#include <kandinsky/camera.h>
#include <kandinsky/graphics/light.h>
#include <kandinsky/graphics/opengl.h>
#include <kandinsky/imgui.h>
#include <kandinsky/memory.h>

#include <random>

namespace kdk {

const char* ToString(EEntityType entity_type) {
    switch (entity_type) {
        case EEntityType::Invalid: return "<INVALID>";
        case EEntityType::Box: return "Box";
        case EEntityType::DirectionalLight: return "DirectionalLight";
        case EEntityType::PointLight: return "PointLight";
        case EEntityType::Spotlight: return "Spotlight";
        case EEntityType::Tower: return "Tower";
        case EEntityType::COUNT: return "<COUNT>";
    }

    ASSERT(false);
    return "<UNKNOWN>";
}

void BuildImgui(const EditorID& editor_id) {
    ImGui::Text("%s - VALUE: %llu", ToString(editor_id.GetEntityType()), editor_id.GetValue());
}

void BuildImgui(const InstanceID& id) {
    ImGui::Text("%s - INDEX: %u, GENERATION: %u", ToString(id.EntityType), id.Index, id.Generation);
}

EditorID GenerateNewEditorID(EEntityType entity_type) {
    // TODO(cdc): Use non C++ apis.

    // Create a random device for seeding
    std::random_device rd;

    // Initialize a Mersenne Twister engine with the random seed
    std::mt19937_64 gen(rd());

    // Create a uniform distribution for 64-bit integers
    std::uniform_int_distribution<uint64_t> dis;

    // Generate random number and combine with entity type in upper 8 bits
    u64 random = dis(gen) & 0x00FFFFFFFFFFFFFF;
    u64 type_bits = (u64)(entity_type) << 56;
    return {random | type_bits};
}

}  // namespace kdk

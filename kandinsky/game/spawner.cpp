#include <kandinsky/game/spawner.h>

#include <kandinsky/imgui.h>
#include <kandinsky/serde.h>

namespace kdk {

void Serialize(SerdeArchive* sa, Spawner& spawner) {
    SERDE(sa, spawner, Entity);
    SERDE(sa, spawner, GridCoord);
}

void Serialize(SerdeArchive*, Enemy&) {
    ASSERTF(false, "We should not be serializing enemies, they are a runtime concept for now");
}

void BuildImGui(Spawner* spawner) {
    // Base entity properties
    BuildImGui(&spawner->Entity);

    // Grid coordinates
    int grid_coord[2] = {static_cast<int>(spawner->GridCoord.x),
                         static_cast<int>(spawner->GridCoord.y)};
    if (ImGui::DragInt2("Grid Position", grid_coord)) {
        spawner->GridCoord.x = grid_coord[0];
        spawner->GridCoord.y = grid_coord[1];
    }
}

void BuildImGui(Enemy* enemy) {
    // Base entity properties
    BuildImGui(&enemy->Entity);
}

}  // namespace kdk

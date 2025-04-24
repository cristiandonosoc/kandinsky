#include <kandinsky/game/tower.h>

#include <kandinsky/game/entity.h>
#include <kandinsky/imgui.h>
#include <kandinsky/serde.h>

namespace kdk {

void BuildImGui(Tower* tower) {
    // Base entity properties
    BuildImGui(&tower->Entity);

    // Grid coordinates
    int grid_coord[2] = {static_cast<int>(tower->GridCoord.x),
                         static_cast<int>(tower->GridCoord.y)};
    if (ImGui::DragInt2("Grid Position", grid_coord)) {
        tower->GridCoord.x = grid_coord[0];
        tower->GridCoord.y = grid_coord[1];
    }

    // Color picker
    float color[4] = {tower->Color.R / 255.0f,
                      tower->Color.G / 255.0f,
                      tower->Color.B / 255.0f,
                      tower->Color.A / 255.0f};
    if (ImGui::ColorEdit4("Color", color)) {
        tower->Color.R = (u8)(color[0] * 255);
        tower->Color.G = (u8)(color[1] * 255);
        tower->Color.B = (u8)(color[2] * 255);
        tower->Color.A = (u8)(color[3] * 255);
    }
}

void Serialize(SerdeArchive* sa, Tower& tower) {
    SERDE(sa, tower, Entity);
    SERDE(sa, tower, GridCoord);
    SERDE(sa, tower, Color);
}

void Serialize(SerdeArchive* sa, Base& base) {
    SERDE(sa, base, Entity);
    SERDE(sa, base, GridCoord);
}

void BuildImGui(Base* base) {
    // Base entity properties
    BuildImGui(&base->Entity);

    // Grid coordinates
    int grid_coord[2] = {static_cast<int>(base->GridCoord.x), static_cast<int>(base->GridCoord.y)};
    if (ImGui::DragInt2("Grid Position", grid_coord)) {
        base->GridCoord.x = grid_coord[0];
        base->GridCoord.y = grid_coord[1];
    }
}

}  // namespace kdk

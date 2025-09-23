#include <imgui.h>
#include <kandinsky/gameplay/health_component.h>

#include <kandinsky/core/serde.h>

namespace kdk {

void Serialize(SerdeArchive* sa, HealthComponent* hc) {
    SERDE(sa, hc, Health);
    SERDE(sa, hc, MaxHealth);
}

void BuildImGui(HealthComponent* hc) {
    float fraction = hc->Health / hc->MaxHealth;
    ImGui::Text("Health %.2f/%.2f", hc->Health, hc->MaxHealth);
    ImGui::SameLine();
    ImGui::ProgressBar(fraction);
}

}  // namespace kdk

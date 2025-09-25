#include <kandinsky/gameplay/health_component.h>

#include <kandinsky/core/serde.h>
#include <kandinsky/imgui.h>
#include <kandinsky/platform.h>

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

bool ReceiveDamage(HealthComponent* health, float amount) {
    health->Health -= amount;
    health->Health = Clamp(health->Health, 0.0f, health->MaxHealth);

    if (health->Health <= 0.0f) {
        DestroyEntity(platform::GetPlatformContext()->EntityManager, health->GetOwnerID());
        return true;
    }

    return false;
}

}  // namespace kdk

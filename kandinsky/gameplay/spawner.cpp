#include <kandinsky/gameplay/spawner.h>

#include <kandinsky/entity.h>
#include <kandinsky/gameplay/enemy.h>
#include <kandinsky/imgui.h>
#include <kandinsky/platform.h>

namespace kdk {

void Update(SpawnerEntity* spawner, float dt) {
    (void)spawner;
    (void)dt;

    auto* ps = platform::GetPlatformContext();
    float now = (float)ps->CurrentTimeTracking->TotalSeconds;

    float target_time = spawner->LastSpawnTime + spawner->SpawnInterval;
    if (now < target_time) {
        return;
    }
    spawner->LastSpawnTime = now;

    CreateEntityOptions options = {
        .Transform = spawner->GetTransform(),
    };

    auto [_, enemy] = CreateEnemy(ps->EntityManager, EEnemyType::Base, options);
    SDL_Log("Entity %s: Spawn entity %s!", spawner->GetName().Str(), enemy->GetName().Str());
}

void Serialize(SerdeArchive* sa, SpawnerEntity* spawner) { SERDE(sa, spawner, SpawnInterval); }

void BuildImGui(SpawnerEntity* spawner) {
    auto* ps = platform::GetPlatformContext();
    float now = (float)ps->CurrentTimeTracking->TotalSeconds;

    ImGui::InputFloat("Spawn Interval", &spawner->SpawnInterval);
    ImGui::Text("Last Spawn Time: %.2f", spawner->LastSpawnTime);
    ImGui::Text("Time Since last spanwn: %.2f", now - spawner->LastSpawnTime);
}

}  // namespace kdk

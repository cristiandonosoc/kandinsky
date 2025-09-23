#include <kandinsky/gameplay/spawner.h>

#include <kandinsky/entity.h>
#include <kandinsky/gameplay/enemy.h>
#include <kandinsky/imgui.h>
#include <kandinsky/platform.h>

namespace kdk {

void Update(Entity* entity, SpawnerEntity* spawner, float dt) {
    (void)entity;
    (void)spawner;
    (void)dt;

    auto* ps = platform::GetPlatformContext();

    float target_time = spawner->LastSpawnTime + spawner->SpawnInterval;
    float now = (float)ps->CurrentTimeTracking->TotalSeconds;
    if (now < target_time) {
        return;
    }
    spawner->LastSpawnTime = now;

    CreateEntityOptions options = {
        .Transform = entity->Transform,
    };

    auto [_, enemy] = CreateEnemy(ps->EntityManager, EEnemyType::Base, options);
    SDL_Log("Entity %s: Spawn entity %s!", entity->Name.Str(), enemy->Name.Str());

}

void Serialize(SerdeArchive* sa, SpawnerEntity* spawner) { SERDE(sa, spawner, SpawnInterval); }

void BuildImGui(SpawnerEntity* spawner) {
    ImGui::InputFloat("Spawn Interval", &spawner->SpawnInterval);
}

}  // namespace kdk

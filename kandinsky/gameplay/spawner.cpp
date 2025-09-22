#include <kandinsky/gameplay/spawner.h>

#include <kandinsky/entity.h>
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

    CreateEntityOptions options = {
        .Transform = entity->Transform,
    };

    auto [_, enemy] = CreateEntity(ps->EntityManager, EEntityType::Enemy, options);
    SDL_Log("Entity %s: Spawn entity %s!", entity->Name.Str(), enemy->Name.Str());

    spawner->LastSpawnTime = now;
}

void Serialize(SerdeArchive* sa, SpawnerEntity* spawner) { SERDE(sa, spawner, SpawnInterval); }

void BuildImGui(SpawnerEntity* spawner) {
    ImGui::InputFloat("Spawn Interval", &spawner->SpawnInterval);
}

}  // namespace kdk

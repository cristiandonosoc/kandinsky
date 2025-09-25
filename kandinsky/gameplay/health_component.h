#pragma once

#include <kandinsky/entity.h>

namespace kdk {

struct HealthComponent
{
	GENERATE_COMPONENT(Health);

	float Health = 100.0f;
	float MaxHealth = 100.0f;
};

void Serialize(SerdeArchive* sa, HealthComponent* hc);
void BuildImGui(HealthComponent* hc);

bool ReceiveDamage(HealthComponent* health, float amount);

} // namespace kdk

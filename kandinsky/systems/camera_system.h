#pragma once

#include <kandinsky/systems/system.h>
#include <kandinsky/camera.h>

namespace kdk {

struct CameraSystem {
	GENERATE_SYSTEM(Camera);

	Camera* Camera = nullptr;
};

void Start(CameraSystem* cs);
void Stop(CameraSystem* cs);
void Update(CameraSystem* cs, float dt);

} // namespace kdk

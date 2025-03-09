#pragma once

#include <kandinsky/graphics/opengl.h>

namespace kdk {

struct PlatformState;

struct TowerDefense {
	static TowerDefense* GetTowerDefense();

    static bool OnSharedObjectLoaded(PlatformState* ps);
    static bool OnSharedObjectUnloaded(PlatformState* ps);
    static bool GameInit(PlatformState* ps);
    static bool GameUpdate(PlatformState* ps);
    static bool GameRender(PlatformState* ps);

    Camera Camera = {
        .Position = {1.0f, 1.0f, 1.0f},
    };
};

}  // namespace kdk

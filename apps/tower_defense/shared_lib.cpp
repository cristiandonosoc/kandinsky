#include <tower_defense/tower_defense.h>

#include <kandinsky/platform.h>

#ifdef __cplusplus
extern "C" {
#endif

namespace kdk {

// clang-format off
bool OnSharedObjectLoaded(PlatformState* ps) { return TowerDefense::OnSharedObjectLoaded(ps); }
bool OnSharedObjectUnloaded(PlatformState* ps) { return TowerDefense::OnSharedObjectUnloaded(ps); }
bool GameInit(PlatformState* ps) { return TowerDefense::GameInit(ps); }
bool GameUpdate(PlatformState* ps) { return TowerDefense::GameUpdate(ps); }
bool GameRender(PlatformState* ps) { return TowerDefense::GameRender(ps); }
// clang-format on

}  // namespace kdk

#ifdef __cplusplus
}  // extern "C"
#endif

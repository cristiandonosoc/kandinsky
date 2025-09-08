#include <kandinsky/platform.h>

namespace kdk {

bool OnSharedObjectLoaded(PlatformState*) { return true; }

bool OnSharedObjectUnloaded(PlatformState*) { return true; }

bool GameInit(PlatformState* ps) {
    (void)ps;
    return true;
}

bool GameUpdate(PlatformState* ps) {
    (void)ps;
    return true;
}

bool GameRender(PlatformState* ps) {
    (void)ps;
    return true;
}

}  // namespace kdk

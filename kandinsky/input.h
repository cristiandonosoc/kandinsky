#pragma once

#include <glm/glm.hpp>

namespace kdk {

struct InputState {
    // Queryable with SDL_SCANLINE_*.
    const bool* KeyboardState = nullptr;
    glm::vec2 MousePosition = {};
    glm::vec2 MouseMove = {};
};
extern InputState* gInputState;

#define INPUT_PRESSED(key) (gInputState->KeyboardState[SDL_SCANCODE_##key])

}  // namespace kdk

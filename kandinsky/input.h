#pragma once

#include <SDL3/SDL_mouse.h>
#include <glm/glm.hpp>

namespace kdk {

struct InputState {
    bool KeyboardOverride = false;  // Normally by imgui.
    bool MouseOverride = false;

    // Queryable with SDL_SCANLINE_*.
    const bool* KeyboardState = nullptr;
    glm::vec2 MousePosition = {};
    glm::vec2 MouseMove = {};
    SDL_MouseButtonFlags MouseState = 0;
};
extern InputState* gInputState;

#define KEY_PRESSED(key) \
    ((bool)(!gInputState->KeyboardOverride && gInputState->KeyboardState[SDL_SCANCODE_##key]))

#define MOUSE_PRESSED(button)              \
    ((bool)(!gInputState->MouseOverride && \
            gInputState->MouseState & SDL_BUTTON_MASK(SDL_BUTTON_##button)))

}  // namespace kdk

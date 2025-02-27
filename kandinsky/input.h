#pragma once

#include <kandinsky/math.h>

#include <SDL3/SDL_mouse.h>
#include <glm/glm.hpp>

namespace kdk {

struct InputState {
    bool KeyboardOverride = false;  // Normally by imgui.
    bool MouseOverride = false;

    // Queryable with SDL_SCANLINE_*.
    const bool* KeyboardState = nullptr;
    Vec2 MousePosition = {};
	// Has the Y = (Height - Y), since OpenGL measures the screen from the upper corner.
	Vec2 MousePositionGL = {};
    Vec2 MouseMove = {};
    SDL_MouseButtonFlags MouseState = 0;
};

#define KEY_PRESSED(ps, key) \
    ((bool)(!ps->InputState.KeyboardOverride && ps->InputState.KeyboardState[SDL_SCANCODE_##key]))

#define MOUSE_PRESSED(ps, button)            \
    ((bool)(!ps->InputState.MouseOverride && \
            ps->InputState.MouseState & SDL_BUTTON_MASK(SDL_BUTTON_##button)))

}  // namespace kdk

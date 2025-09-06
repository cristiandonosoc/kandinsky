#pragma once

#include <kandinsky/core/math.h>

#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_scancode.h>

#include <array>
#include <bitset>

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

    // Set of mouse buttons pressed this frame.
    // IMPORTANT: Must be aligned with SDl_MouseButtonFlags.
    std::bitset<SDL_BUTTON_X2> MousePressed = {};
    std::bitset<SDL_BUTTON_X2> MouseDown = {};
    std::bitset<SDL_BUTTON_X2> MouseReleased = {};

    std::bitset<(u32)SDL_SCANCODE_COUNT> KeyPressed = {};
    std::bitset<(u32)SDL_SCANCODE_COUNT> KeyReleased = {};
};

#define KEY_PRESSED(ps, key) \
    ((bool)(!ps->InputState.KeyboardOverride && ps->InputState.KeyPressed[(SDL_SCANCODE_##key)]))

#define KEY_DOWN(ps, key) \
    ((bool)(!ps->InputState.KeyboardOverride && ps->InputState.KeyboardState[SDL_SCANCODE_##key]))

#define MOUSE_PRESSED(ps, button) \
    ((bool)(!ps->InputState.MouseOverride && ps->InputState.MousePressed[(SDL_BUTTON_##button)]))

#define MOUSE_DOWN(ps, button) \
    ((bool)(!ps->InputState.MouseOverride && ps->InputState.MouseDown[(SDL_BUTTON_##button)]))

#define MOUSE_RELEASED(ps, button) \
    ((bool)(!ps->InputState.MouseOverride && ps->InputState.MouseReleased[(SDL_BUTTON_##button)]))

}  // namespace kdk

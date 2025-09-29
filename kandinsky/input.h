#pragma once

#include <kandinsky/core/math.h>

#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_scancode.h>

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

    std::bitset<(u32)SDL_SCANCODE_COUNT> KeyPressed = {};
    std::bitset<(u32)SDL_SCANCODE_COUNT> KeyDown = {};
    std::bitset<(u32)SDL_SCANCODE_COUNT> KeyReleased = {};

    // Set of mouse buttons pressed this frame.
    // IMPORTANT: Must be aligned with SDl_MouseButtonFlags.
    std::bitset<SDL_BUTTON_X2> MousePressed = {};
    std::bitset<SDL_BUTTON_X2> MouseDown = {};
    std::bitset<SDL_BUTTON_X2> MouseReleased = {};
};

// The _IMGUI variants are meant to ignore the override, so will be true even in imgui.
// Normal gameplay should not call these.

#define KEY_PRESSED(ps, key) \
    ((bool)(!ps->InputState.KeyboardOverride && ps->InputState.KeyPressed[(SDL_SCANCODE_##key)]))
#define KEY_PRESSED_IMGUI(ps, key) ((bool)(ps->InputState.KeyPressed[(SDL_SCANCODE_##key)]))

#define KEY_DOWN(ps, key) \
    ((bool)(!ps->InputState.KeyboardOverride && ps->InputState.KeyDown[SDL_SCANCODE_##key]))
#define KEY_DOWN_IMGUI(ps, key) ((bool)(ps->InputState.KeyDown[SDL_SCANCODE_##key]))

#define KEY_RELEASED(ps, key) \
    ((bool)(!ps->InputState.KeyboardOverride && ps->InputState.KeyReleased[SDL_SCANCODE_##key]))
#define KEY_RELEASED_IMGUI(ps, key) ((bool)(ps->InputState.KeyReleased[SDL_SCANCODE_##key]))

#define MOUSE_PRESSED(ps, button) \
    ((bool)(!ps->InputState.MouseOverride && ps->InputState.MousePressed[(SDL_BUTTON_##button)]))
#define MOUSE_PRESSED_IMGUI(ps, button) ((bool)(ps->InputState.MousePressed[(SDL_BUTTON_##button)]))

#define MOUSE_DOWN(ps, button) \
    ((bool)(!ps->InputState.MouseOverride && ps->InputState.MouseDown[(SDL_BUTTON_##button)]))
#define MOUSE_DOWN_IMGUI(ps, button) ((bool)(ps->InputState.MouseDown[(SDL_BUTTON_##button)]))

#define MOUSE_RELEASED(ps, button) \
    ((bool)(!ps->InputState.MouseOverride && ps->InputState.MouseReleased[(SDL_BUTTON_##button)]))
#define MOUSE_RELEASED_IMGUI(ps, button) \
    ((bool)(ps->InputState.MouseReleased[(SDL_BUTTON_##button)]))

#define CTRL_DOWN(ps) (KEY_DOWN(ps, LCTRL) || KEY_DOWN(ps, RCTRL))
#define CTRL_DOWN_IMGUI(ps) (KEY_DOWN_IMGUI(ps, LCTRL) || KEY_DOWN_IMGUI(ps, RCTRL))

#define ALT_DOWN(ps) (KEY_DOWN(ps, LALT) || KEY_DOWN(ps, RALT))
#define ALT_DOWN_IMGUI(ps) (KEY_DOWN_IMGUI(ps, LALT) || KEY_DOWN_IMGUI(ps, RALT))

#define SHIFT_DOWN(ps) (KEY_DOWN(ps, LSHIFT) || KEY_DOWN(ps, RSHIFT))
#define SHIFT_DOWN_IMGUI(ps) (KEY_DOWN_IMGUI(ps, LSHIFT) || KEY_DOWN_IMGUI(ps, RSHIFT))

}  // namespace kdk

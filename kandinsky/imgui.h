#pragma once

#include <imgui.h>

namespace kdk {

struct PlatformState;

bool InitImgui(PlatformState* ps);
void ShutdownImgui(PlatformState* ps);
void BeginImguiFrame();
void RenderImgui();

} // namespace kdk

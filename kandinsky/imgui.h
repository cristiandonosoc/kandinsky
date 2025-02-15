#pragma once

#include <imgui.h>

namespace kdk {

bool InitImgui();
void ShutdownImgui();
void BeginImguiFrame();
void RenderImgui();

} // namespace kdk

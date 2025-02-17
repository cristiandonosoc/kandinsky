#pragma once

#include <imgui.h>

namespace kdk {

struct PlatformState;

bool InitImgui(PlatformState* ps);
void ShutdownImgui(PlatformState* ps);
void BeginImguiFrame();
void RenderImgui();

void* ImguiMalloc(size_t size, void* user_data);
void ImguiFree(void* ptr, void* user_data);

}  // namespace kdk

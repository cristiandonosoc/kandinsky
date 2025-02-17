#include <kandinsky/imgui.h>

#include <kandinsky/game.h>
#include <kandinsky/window.h>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

namespace kdk {

bool InitImgui(PlatformState* ps) {
    if (!IsValid(ps->Window)) {
        SDL_Log("ERROR: Imgui initialization. Window is not valid. Did you call InitWindow?");
        return false;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(ps->Window.SDLWindow, ps->Window.GLContext);
    ImGui_ImplOpenGL3_Init(nullptr);  // Let the platform decide version.

    ps->ImguiContext = ImGui::GetCurrentContext();
    ps->ImguiAllocFunc = ImguiMalloc;
    ps->ImguiFreeFunc = ImguiFree;

    return true;
}

void ShutdownImgui(PlatformState*) {
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

void BeginImguiFrame() {
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void RenderImgui() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void* ImguiMalloc(size_t size, void*) { return malloc(size); }
void ImguiFree(void* ptr, void*) { free(ptr); }

}  // namespace kdk

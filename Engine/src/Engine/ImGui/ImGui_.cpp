#include "pch.h"
#include "ImGui_.h"
#include "../vendor/imgui/backends/imgui_impl_win32.cpp"
#include "../vendor/imgui/backends/imgui_impl_dx12.cpp"

ImGui_::ImGui_()
{
}

ImGui_::~ImGui_()
{
    // Cleanup
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    // TODO: Properly shutdown imgui
    //ImGui::DestroyContext(ctx);
}

void ImGui_::Init()
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

}

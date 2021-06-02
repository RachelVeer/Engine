#pragma once

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_dx12.h>

class DearImGui
{
public:
    DearImGui()
    {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize.x = 1920.0f;
        io.DisplaySize.y = 1080.0f;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
    }
};
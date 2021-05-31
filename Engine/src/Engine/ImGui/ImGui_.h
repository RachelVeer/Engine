#pragma once
#include "imgui.h"
//#include "Engine/ImGui/Direct3D12ImGui.h"
#include "../vendor/imgui/backends/imgui_impl_win32.h"
#include "../vendor/imgui/backends/imgui_impl_dx12.h"

class ImGui_
{
public:
    ImGui_();
    ~ImGui_();
    static void Init();
};
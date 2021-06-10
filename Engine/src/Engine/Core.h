#pragma once
#include "Log.h"
#include "ImGuiLocal/ImGuiLocal.h"

class Core
{
public:
    Core()
    {
        // Logger.
        Log::Init();
        ENGINE_CORE_WARN("Initialized Logger!");
        // int a = 5;
        // ENGINE_INFO("Test: Hello! Var={0}", a);
        
        // Immediate interface
        DearImGui::Init();
    }
};
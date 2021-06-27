module;
#include "Log.h"
#include "ImGuiLocal/ImGuiLocal.h"
export module Core;

// This module serves as a bridge between modularized code & external header intense libraries. 

export void Core()
{
    // Logger.
    Log::Init();
    ENGINE_CORE_INFO("Initialized Logger!");
    // int a = 5;
    // ENGINE_INFO("Test: Hello! Var={0}", a);

    // Immediate interface
    DearImGui::Init();

    ENGINE_CORE_INFO("Core.ixx successfully called from!");
}
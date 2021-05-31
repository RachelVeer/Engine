#pragma once
#include "Log.h"
#include "ImGui/ImGui_.h"

class Core
{
public:
    Core()
    {
        // Logger.
        Log::Init();
        ENGINE_CORE_WARN("Initialized Logger!\n");
        // int a = 5;
        // ENGINE_INFO("Test: Hello! Var={0}\n", a);
        ImGui_::Init();
    }
};

// If we're on Windows, allow DirectX to be defined. 
#if defined(WIN32) || defined(_WIN32) || defined(_WIN64) 
    #define DirectX
    #define D3D
    #define D3D12
#endif
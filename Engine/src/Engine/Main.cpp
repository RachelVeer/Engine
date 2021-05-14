//*********************************************************
// Copyright (c) 2021 Rachel Veer.
// Licensed under the Apache-2.0 License.
//*********************************************************

#include "pch.h"

#include "Platform/Platform.h"
#include "Platform/Direct3D/D3D12Context.h"

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ PWSTR pCmdLine,
    _In_ int nCmdShow)
{
    // Configure current platform. 
    Platform::PlatformState state = {};
    Platform platform = {};
    
    platform.PlatformStartup(&state, L"Seacrest", 100, 100, 1280, 720);
    
    while (platform.platformRunning)
    {
        platform.PlatformPumpMessages(&state);
    }

    platform.PlatformShutdown(&state);

    // Init D3D12.
    //Direct3D direct3d();

    return 0;
}
//*********************************************************
// Copyright (c) 2021 Rachel Veer.
// Licensed under the Apache-2.0 License.
//*********************************************************

#include "pch.h"

#include <iostream>

#include "Platform/Platform.h"
#include "Platform/Direct3D/D3D12Context.h"

int main()
{
    std::cout << "This is a test." << '\n';

    // Configure current platform. 
    Platform::PlatformState state = {};
    Platform platform = {};
    
    platform.PlatformStartup(&state, L"Seacrest", 100, 100, 1280, 720);
    
    // Init D3D12.
    Direct3D direct3d;

    while (platform.platformRunning)
    {
        platform.PlatformPumpMessages(&state);
        direct3d.OnUpdate();
        direct3d.OnRender();
    }

    platform.PlatformShutdown(&state);

    return 0;
}
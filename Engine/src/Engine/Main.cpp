//*********************************************************
// Copyright (c) 2021 Rachel Veer.
// Licensed under the Apache-2.0 License.
//*********************************************************

#include "pch.h"

#include <iostream>

#include "Platform/Platform.h"
#include "Platform/Direct3D/D3D12Context.h"

static void removeTrailingCharacters(std::string& str) {
    str.erase(4, std::string::npos);
}


int main()
{
    std::cout << "This is a test." << '\n';

    // Configure current platform. 
    Platform::PlatformState state = {};
    Platform* platform;
    platform = platform->Create();
    
    platform->Startup(&state, L"Seacrest", 100, 100, 1280, 720);
    
    // Init D3D12.
    Direct3D direct3d;
    
    while (platform->IsRunning())
    {
        auto test = platform->Peek();
        std::string s = std::to_string(test);
        removeTrailingCharacters(s);
        std::cout << "Application's life-time: " << s << "/s" << '\r';

        platform->PumpMessages(&state);
        
        direct3d.OnUpdate();
        direct3d.OnRender();
    }

    platform->Shutdown(&state);

    return 0;
}
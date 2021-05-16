//*********************************************************
// Copyright (c) 2021 Rachel Veer.
// Licensed under the Apache-2.0 License.
//*********************************************************

#include "pch.h"

#include <iostream>
#include <thread>

#include "Platform/Platform.h"
#include "Platform/Direct3D/D3D12Context.h"

static void removeTrailingCharacters(std::string& str) {
    str.erase(4, std::string::npos);
}

// Configure current platform. 
Platform::PlatformState state = {};
Platform* platform;

bool peeking = true;

void DoTime()
{
    while (peeking)
    {
        auto test = platform->Peek();
        std::string s = std::to_string(test);
        removeTrailingCharacters(s);
        std::cout << "Application's life-time: " << s << "/s" << '\r';
    }
}

int main()
{
    std::cout << "This is a test." << '\n';

    platform = platform->Create();
    
    platform->Startup(&state, L"Seacrest", 100, 100, 1280, 720);
    
    // Platform setups time, thus time
    // thread comes after its initialization.
    std::thread timer(DoTime);

    // Init D3D12.
    Direct3D direct3d;

    while (platform->IsRunning())
    {
        platform->PumpMessages(&state);
        direct3d.OnUpdate();
        direct3d.OnRender();
    }
    
    // Make sure to break separate while loop and suspend thread 
    // before shutting application down.  
    peeking = false;
    timer.join();

    platform->Shutdown(&state);

    return 0;
}
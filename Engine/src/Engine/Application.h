#pragma once

#include <thread>

#include "Platform/Platform.h"
#include "Platform/Direct3D/D3D12Context.h"

class Application
{
public:
    Application();
    ~Application();
    void Create();
    void Run();
    void Shutdown();
    void DoTime();
private:
    bool m_Peeking;
    bool m_Running;
    std::thread thread;
    std::unique_ptr<Platform> m_Platform;
    // Configure current platform. 
    Platform::PlatformState state = {};
};
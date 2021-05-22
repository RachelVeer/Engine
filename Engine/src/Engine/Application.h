#pragma once

#include <thread>

#include "Platform/Platform.h"
#include "Platform/Direct3D/D3D12Context.h"

class Application
{
public:
    Application() = default;
    ~Application() = default;
    void Create();
    void Run();
    void Shutdown();
    void DoTime();
};
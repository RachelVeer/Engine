#include "pch.h"
#include "Application.h"

#include "EntryPoint.h"

Platform* platform;

Application::Application()
    :m_Peeking(true), m_Running(true)
{}

Application::~Application()
{}

void Application::Create()
{
    std::cout << "This is a test." << '\n';

    platform->Startup(L"Seacrest", 200, 250, 1280, 720);

    // Platform setups time, thus time
    // thread comes after its initialization.
    m_ThreadTimer = std::thread(&Application::DoTime, this);
}

void Application::Run()
{
    // Temporary start of D3D.
    Direct3D direct3d;

    while (m_Running)
    {
        // Exit code (ecode) is only processed from platform-side Quit message.
        if(const auto ecode = platform->PumpMessages()) {
            if (ecode) {
                m_Running = false;
            }
        }
        direct3d.OnUpdate();
        direct3d.OnRender();
    }
}

void Application::Shutdown()
{
    // Make sure to break separate while loop and suspend thread 
    // before shutting application down.  
    m_Peeking = false;
    m_ThreadTimer.join();

    platform->Shutdown();
}

void Application::DoTime()
{
    // To "peek" is to get a glimpse at time. 
    while (m_Peeking)
    {
        auto elapsedTime = platform->Peek();
        // The results of Peek() undergo formatting for readablitiy.
        printf("Application's life-time %.2f \r", elapsedTime);
    }
}

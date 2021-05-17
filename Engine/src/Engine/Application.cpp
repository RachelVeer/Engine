#include "pch.h"
#include "Application.h"

static void removeTrailingCharacters(std::string& str) {
    str.erase(4, std::string::npos);
}

Application::Application()
    :m_Peeking(true)
{}

Application::~Application()
{}

void Application::Create()
{
    std::cout << "This is a test." << '\n';

    m_Platform = m_Platform->Create();

    m_Platform->Startup(&state, L"Seacrest", 100, 100, 1280, 720);
}

void Application::Run()
{
    // Platform setups time, thus time
    // thread comes after its initialization.
    thread = std::thread(&Application::DoTime, this);

    Direct3D direct3d;

    while (m_Platform->IsRunning())
    {
        m_Platform->PumpMessages(&state);
        direct3d.OnUpdate();
        direct3d.OnRender();
    }

    // Make sure to break separate while loop and suspend thread 
    // before shutting application down.  
    m_Peeking = false;
    thread.join();

    m_Platform->Shutdown(&state);
}

void Application::DoTime()
{
    while (m_Peeking)
    {
        auto test = m_Platform->Peek();
        std::string s = std::to_string(test);
        removeTrailingCharacters(s);
        std::cout << "Application's life-time: " << s << "/s" << '\r';
    }
}

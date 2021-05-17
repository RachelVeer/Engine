#include "pch.h"
#include "Application.h"

static void removeTrailingCharacters(std::string& str) {
    str.erase(4, std::string::npos);
}

Application::Application()
    :m_Peeking(true), m_Running(true)
{}

Application::~Application()
{}

void Application::Create()
{
    std::cout << "This is a test." << '\n';

    m_Platform = m_Platform->Create();

    m_Platform->Startup(L"Seacrest", 100, 100, 1280, 720);

    // Platform setups time, thus time
    // thread comes after its initialization.
    thread = std::thread(&Application::DoTime, this);
}

void Application::Run()
{
    // Temporary start of D3D.
    Direct3D direct3d;

    while (m_Running)
    {
        // Exit code (ecode) is only processed from platform-side Quit message.
        if(const auto ecode = m_Platform->PumpMessages()) {
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
    thread.join();

    m_Platform->Shutdown();
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

#include "pch.h"
#include "Application.h"

typedef std::thread thread;

typedef struct ApplicationState
{
    bool Initialized;
    bool Running;
    thread ThreadTimer;
    double ElapsedTime;
} ApplicationState;

static ApplicationState appState;
Platform* platform;

void Application::Create()
{
    printf("This is a test.\n");

    appState.Running = true;

    printf("Application State Running?: ");
    printf(appState.Running? "true\n" : "false\n");

    platform->Startup(L"Seacrest", 200, 250, 1280, 720);

    // Platform setups time, thus time
    // thread comes after its initialization.
    appState.ThreadTimer = std::thread(&Application::DoTime, this);

    appState.Initialized = true;
}

void Application::Run()
{
    if (appState.Initialized)
    {
        // Temporary start of D3D.
        Direct3D direct3d;

        while (appState.Running)
        {
            // Exit code (ecode) is only processed from platform-side Quit message.
            if (const auto ecode = platform->PumpMessages()) {
                if (ecode) {
                    appState.Running = false;
                }
            }
            direct3d.OnUpdate();
            direct3d.OnRender();
        }
    }
}

void Application::Shutdown()
{
    // Make sure to break separate while loop and suspend thread 
    // before shutting application down.  
    if (appState.Running)
    {
        appState.Running = false;
    }

    appState.ThreadTimer.join();

    platform->Shutdown();
}

void Application::DoTime()
{
    // To "peek" is to get a glimpse at time. 
    while (appState.Running)
    {
        auto elapsedTime = platform->Peek();
        // The results of Peek() undergo formatting for readablitiy.
        printf("Application's life-time %.2f \r", elapsedTime);
    }
    printf("\nDoTime Thread Shutting down.\n");
}

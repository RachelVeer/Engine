#include "pch.h"
#include "Application.h"
#include "SandboxTypes.h"
#include "GraphicsContext.h"

#include <fstream>

typedef std::thread thread;

typedef struct ApplicationState
{
    SandboxState* sandboxInstance = {};
    bool Initialized = false;
    bool Running = false;
    thread ThreadTimer;
    double ElapsedTime = { 0.0 };
} ApplicationState;

static ApplicationState appState;
Platform* platform;
Graphics* gfx;

void Application::Create(SandboxState* sandboxInstance)
{
    printf("This is a test.\n");

    appState.sandboxInstance = sandboxInstance;
    appState.Running = true;
    
    platform->Startup(
        sandboxInstance->appConfig.Name,
        sandboxInstance->appConfig.startPosX,
        sandboxInstance->appConfig.startPosY,
        sandboxInstance->appConfig.startWidth,
        sandboxInstance->appConfig.startHeight);

    gfx = gfx->CreateGraphics();
    gfx->Init();
    // Platform setups time, thus time
    // thread comes after its initialization.
    appState.ThreadTimer = std::thread(&Application::DoTime, this);

    appState.Initialized = true;
}

void Application::Run()
{
    printf("Application State Running?: ");
    printf(appState.Running ? "true\n" : "false\n");

    if (appState.Initialized)
    {
        while (appState.Running)
        {
            // Exit code (ecode) is only processed from platform-side Quit message.
            if (const auto ecode = platform->PumpMessages()) {
                if (ecode) {
                    appState.Running = false;
                }
            }
            gfx->Update();
            gfx->Render();
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
    delete platform;
    delete gfx;
}

void Application::DoTime()
{
    // To "peek" is to get a glimpse at time. 
    while (appState.Running)
    {
        appState.ElapsedTime = platform->Peek();
        // The results of Peek() undergo formatting for readablitiy.
        printf("Application's life-time %.2f \r", appState.ElapsedTime);
    }

    std::ofstream myFile;
    myFile.open("example.txt");
    printf("\nWriting elapsedTime to a file.\n");
    printf("Elapsed Time: %.2f", appState.ElapsedTime);
    myFile.close();

    printf("\nDoTime Thread Shutting down.\n");
}

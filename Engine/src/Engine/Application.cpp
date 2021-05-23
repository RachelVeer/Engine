#include "pch.h"
#include "Application.h"
#include "GameTypes.h"

#include <fstream>

typedef std::thread thread;

typedef struct ApplicationState
{
    Game* gameInstance;
    bool Initialized = false;
    bool Running = false;
    thread ThreadTimer;
    double ElapsedTime = { 0.0 };
} ApplicationState;

static ApplicationState appState;
Platform* platform;

void Application::Create(Game* gameInstance)
{
    printf("This is a test.\n");

    appState.gameInstance = gameInstance;
    appState.Running = true;
    
    platform->Startup(
        gameInstance->appConfig.Name, 
        gameInstance->appConfig.startPosX,
        gameInstance->appConfig.startPosY,
        gameInstance->appConfig.startWidth,
        gameInstance->appConfig.startHeight);

    // Platform setups time, thus time
    // thread comes after its initialization.
    appState.ThreadTimer = std::thread(&Application::DoTime, this);

    appState.gameInstance->Initialize(appState.gameInstance);

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

            appState.gameInstance->Update(appState.gameInstance, 0.0f);
            appState.gameInstance->Render(appState.gameInstance, 0.0f);
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
    std::cout << "\nWriting elapsedTime to a file.\n" << "Elapsed Time: "
        << appState.ElapsedTime;
    myFile.close();

    printf("\nDoTime Thread Shutting down.\n");
}

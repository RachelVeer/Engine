#include "pch.h"
#include "Application.h"
#include "SandboxTypes.h"
#include "Engine/GraphicsContext.h"
#include "Log.h"

// Dear ImGui functionality.
#include "ImGuiLocal/ImGuiLocal.h"

#include <fstream> // For file functions.


typedef std::thread Thread;

struct ApplicationState
{
    SandboxState* sandboxInstance = {};
    bool Initialized   = false;
    bool Running       = false;
    double ElapsedTime = { 0.0 };
    Thread ThreadTimer;
};

static ApplicationState appState;

// Interfaces. 
Platform platform;
Graphics gfx;
DearImGui imgui;

void Application::Create(SandboxState* sandboxInstance)
{
    ENGINE_CORE_INFO("This is a test.\n");

    // Retrieve the original Sandbox instance & store it here.
    appState.sandboxInstance = sandboxInstance;

    // If instance successfully retrieved, we're 
    // officially up and running at this point. 
    appState.Running = true;

    ENGINE_CORE_WARN("Application State Running?: \n");
    ENGINE_CORE_DEBUG(appState.Running ? "true\n" : "false\n");

    // App in this case simply encapsulates its configuration.
    auto app = appState.sandboxInstance->appConfig;
    
    platform.Startup(
        app.Name,
        app.startPosX,
        app.startPosY,
        app.startWidth,
        app.startHeight);

    // Platform setups time, thus time
    // thread comes after its initialization.
    appState.ThreadTimer = std::thread(&Application::DoTime, this);
    
    // Initiate the actual graphics pipeline. 
    gfx.Init();

    // Assuming all functions have succeeded, reaching the end
    // of this function signals the application has initialized.
    appState.Initialized = true;
}

void Application::Run()
{
    ClearColor color = { 1.0f, 0.3f, 0.4f, 1.0f };
    bool show_demo_window = true;

    if (appState.Initialized)
    {
        while (appState.Running)
        {
            // Exit code (ecode) is only processed from platform-side Quit message.
            if (const auto ecode = platform.PumpMessages()) {
                if (ecode) {
                    appState.Running = false;
                }
            }
            else            
            {
                imgui.BeginFrame();
                imgui.DemoWindows(color, show_demo_window);

                // Creating our own imgui stuff rather than just default code.
                {
                    ImGui::Begin("Our Demo");
                    // Track application life-time.
                    ImGui::Text("Application life-time: = %.2f /s", appState.ElapsedTime);

                    // Track mouse coords.
                    int16_t x = platform.GetXScreenCoordinates();
                    int16_t y = platform.GetYScreenCoordinates();
                    ImGui::Text("Mouse X coords: = %d", x);
                    ImGui::Text("Mouse Y coords: = %d", y);
                    ImGui::End();
                }

                // Rendering
                imgui.EndFrame(); // Actually render imgui setup
                gfx.Update();
                gfx.Render(color);
            }
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

    // Join the timer with the main thread to cleanly shutdown.
    // (So it isn't still running and/or forcefully cut off).
    appState.ThreadTimer.join();
    // Cleanup interfaces.
    platform.Shutdown();
    gfx.Shutdown();
    imgui.Shutdown();
}

void Application::DoTime()
{
    // To "peek" is to get a glimpse at time. 
    while (appState.Running)
    {
        appState.ElapsedTime = platform.Peek();
        // The results of Peek() undergo formatting for readablitiy.
        // ENGINE_CORE_DEBUG("Application's life-time: {:.2f} \r", appState.ElapsedTime);
    }

    // TODO(rachel): Make this a function. 
    {
        std::ofstream myFile;
        myFile.open("example.txt");
        printf("\n");
        ENGINE_CORE_INFO("Writing elapsedTime to a file.\n");
        // This is what's actually written to the file.
        myFile << "Elapsed Time: "
            << appState.ElapsedTime;
        myFile.close();
    }

    ENGINE_CORE_INFO("DoTime Thread Shutting down.\n");
}

#include "Application.h"
#include "SandboxTypes.h"
#include "Engine/GraphicsContext.h"

#include <fstream> // For file functions.
#include "LogDependencies.h"


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

import Log;
import ImGuiLocal;


void Application::Create(SandboxState* sandboxInstance)
{
    // Retrieve the original Sandbox instance & store it here.
    appState.sandboxInstance = sandboxInstance;

    int a = 5;
    double c = 4.5;

    // Initialize sub-systems. 
    //Core();
    LogInit();
    ImGuiLocal::Init();
    //s_CoreLogger->trace("This is from the module.");
    GetCoreLogger()->trace("GetCoreLogger() Test.");
    Trace("Trace() test. Var={0}", a);
    Trace("Trace() test. Var={0}", c);
    //int b = 5;
    //Trace("Yes. Var={0}", b);

    // If instance successfully retrieved, we're 
    // officially up and running at this point. 
    appState.Running = true;

    //ENGINE_CORE_WARN("Application State Running?: ");
    //ENGINE_CORE_DEBUG(appState.Running ? "true" : "false");
    //logger.GetCoreLogger()->trace("This is a test spdlog print without a macro. Var={0}", a);

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
    gfx.Init(app.startWidth, app.startHeight);

    // Assuming all functions have succeeded, reaching the end
    // of this function signals the application has initialized.
    appState.Initialized = true;
}

void Application::Run()
{
    ClearColor color = { 0.086f, 0.086f, 0.086f, 1.0f }; // #161616
    bool show_demo_window = false;
    static int counter = { 0 };
    bool updatingClearColor = false;
    bool adjustOffset = false;

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
                ImGuiLocal::BeginFrame();
                ImGuiLocal::DemoWindows(color, show_demo_window);

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

                    if (ImGui::Button("Screenshot"))
                    {
                        // Buttons return true when clicked (most widgets return true when edited/activated)
                        gfx.Screenshot();
                        counter++;
                    }
                    ImGui::SameLine();
                    ImGui::Text("Screenshots taken: = %d", counter);
                    ImGui::End();
                }

                {
                    ImGui::Begin("Resource State.");
                    ImGui::Checkbox("Clear color over time", &updatingClearColor);
                    ImGui::Checkbox("Enable constant (movement)", &adjustOffset);
                    ImGui::End();
                    // Update clear color.
                    if (updatingClearColor)
                    {
                        float timeValue = static_cast<float>(platform.GetAbsoluteTime());
                        float greenValue = sin(timeValue) / 2.0f + 0.5f;
                        color.g = greenValue;
                    }
                }
               
                // Rendering
                ImGuiLocal::EndFrame(); // Actually render imgui setup
                gfx.Update(color, adjustOffset);
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
    ImGuiLocal::Shutdown();
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
        //ENGINE_CORE_INFO("Writing elapsedTime to a file.");
        // This is what's actually written to the file.
        myFile << "Elapsed Time: "
            << appState.ElapsedTime;
        myFile.close();
    }

    //ENGINE_CORE_INFO("DoTime Thread Shutting down.");
}

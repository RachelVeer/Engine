#include "pch.h"
#include "Application.h"
#include "SandboxTypes.h"
#include "Engine/GraphicsContext.h"
#include "Log.h"
#include "Engine/ImGui/ImGui_.h"

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
ImGui_ imgui;

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
    bool show_demo_window = true;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

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
            // Start the Dear ImGui frame
            ImGui_ImplDX12_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
            if (show_demo_window)
                ImGui::ShowDemoWindow(&show_demo_window);

            // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
            {
                static float f = 0.0f;
                static int counter = 0;

                ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

                ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
                ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state

                ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
                ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

                if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                    counter++;
                ImGui::SameLine();
                ImGui::Text("counter = %d", counter);

                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                ImGui::End();
            }

            // Rendering
            ImGui::Render();



            gfx.Update();
            gfx.Render();
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
    platform.Shutdown();
    imgui.~ImGui_();
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

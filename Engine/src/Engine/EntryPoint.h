//*********************************************************
// Copyright (c) 2021 Rachel Veer.
// Licensed under the Apache-2.0 License.
//*********************************************************

#include "pch.h"
#include "Core.h"
#include "Application.h"
#include "SandboxTypes.h"

// Externally defined function to create a sandbox.
extern Application* CreateApplication(SandboxState* OutSandbox);

int main(int argc, char* argv[])
{
    int minimalArgumentCount = { 1 };

    // Relative path in regards to Sandbox.
    int relativePathCharacters = { 36 };

    // argv[0] should be the name of the program,
    // thus there should always be at least one count present.
    if (argc >= minimalArgumentCount)
    {
        // Initialize sub-systems. 
        Core();

        std::string exeName = argv[0];            // Assign arg char to string.
        exeName.erase(0, relativePathCharacters); // Erase relative path.
        ENGINE_CORE_INFO("Current Executable: {0}\n", exeName.c_str());

        // Request the sandbox instance from the application.
        SandboxState sandboxInstance;
        // Define & allocate memory for application.
        auto app = CreateApplication(&sandboxInstance);
        
        app->Create(&sandboxInstance);
        app->Run();
        // In any event where the application  
        // loop is broken out of - shutdown. 
        app->Shutdown();
        delete app;
    }

    return 0;
}
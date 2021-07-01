//*********************************************************
// Copyright (c) 2021 Rachel Veer.
// Licensed under the Apache-2.0 License.
//*********************************************************

#include <filesystem>

import Application;

// Externally defined function to create a sandbox.
extern Application* CreateApplication(SandboxState* OutSandbox);

int main(int argc, char* argv[])
{
    int minimalArgumentCount = { 1 };

    // argv[0] should be the name of the program,
    // thus there should always be at least one count present.
    if (argc >= minimalArgumentCount)
    {
        auto exeName = std::filesystem::path(argv[0]).filename();
        printf("Launched Executable: %s\n", exeName.string().c_str());
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
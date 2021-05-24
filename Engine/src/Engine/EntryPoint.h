//*********************************************************
// Copyright (c) 2021 Rachel Veer.
// Licensed under the Apache-2.0 License.
//*********************************************************

#include "pch.h"
#include "Application.h"
#include "SandboxTypes.h"

// Externally defined function to create a sandbox.
extern void CreateSandbox(Sandbox* OutSandbox);
// Define application.
Application app;

int main()
{      
    // Request the sandbox instance from the application.
    Sandbox sandboxInstance;
    CreateSandbox(&sandboxInstance);

    // Ensure the function pointers exist.
    if (!sandboxInstance.Render || !sandboxInstance.Update || !sandboxInstance.Initialize)
    {
        return -1;
    }

    app.Create(&sandboxInstance);
    app.Run();
    // In any event where the application  
    // loop is broken out of - shutdown. 
    app.Shutdown();

    return 0;
}
//*********************************************************
// Copyright (c) 2021 Rachel Veer.
// Licensed under the Apache-2.0 License.
//*********************************************************

#include "pch.h"
#include "Application.h"
#include "SandboxTypes.h"

// Externally defined function to create a sandbox.
extern Application* CreateApplication(SandboxState* OutSandbox);

int main()
{
    Log::Init();
    ENGINE_CORE_WARN("Initialized Info!");
    int a = 5;
    ENGINE_INFO("Hello! Var={0}", a);
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

    return 0;
}
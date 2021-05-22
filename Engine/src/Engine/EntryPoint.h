//*********************************************************
// Copyright (c) 2021 Rachel Veer.
// Licensed under the Apache-2.0 License.
//*********************************************************

#include "pch.h"
#include "Application.h"

extern void Sandbox();

int main()
{   
    Application app;
    
    // While Sandbox is only test code for now, this will 
    // eventually serve to separate game from engine code. 
    Sandbox();

    app.Create();
    app.Run();
    // In any event where the application  
    // loop is broken out of - shutdown. 
    app.Shutdown();

    return 0;
}
//*********************************************************
// Copyright (c) 2021 Rachel Veer.
// Licensed under the Apache-2.0 License.
//*********************************************************

#include "pch.h"

int main(void)
{   
    Application app;

    app.Create();
    app.Run();
    // In any event where the application  
    // loop is broken out of - shutdown. 
    app.Shutdown();

    return 0;
}
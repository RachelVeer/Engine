//*********************************************************
// Copyright (c) 2021 Rachel Veer.
// Licensed under the Apache-2.0 License.
//*********************************************************

#include "pch.h"
#include "Application.h"
#include "GameTypes.h"

// Externally defined function to create a game.
extern void CreateGame(Game* OutGame);
// Define application.
Application app;

int main()
{      
    // Request the game instance from the application.
    Game gameInstance;
    CreateGame(&gameInstance);

    // Ensure the function pointers exist.
    if (!gameInstance.Render || !gameInstance.Update || !gameInstance.Initialize)
    {
        return -1;
    }

    app.Create(&gameInstance);
    app.Run();
    // In any event where the application  
    // loop is broken out of - shutdown. 
    app.Shutdown();

    return 0;
}
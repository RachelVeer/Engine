#include "Sandbox.h"
#include <EntryPoint.h>

// Define the function to create a game.
void CreateGame(Game* OutGame)
{
    // Application configuration.
    OutGame->appConfig.startPosX = 100;
    OutGame->appConfig.startPosY = 100;
    OutGame->appConfig.startWidth = 1280;
    OutGame->appConfig.startHeight = 720;
    OutGame->appConfig.Name = L"Seacrest Engine Sandbox";
    OutGame->Update = SandboxUpdate;
    OutGame->Render = SandboxRender;
    OutGame->Initialize = SandboxInitialize;

    // Create the game state.
    OutGame->state = malloc(sizeof(SandboxState));
}
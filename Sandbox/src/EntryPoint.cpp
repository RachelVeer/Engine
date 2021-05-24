#include "Sandbox.h"
#include <EntryPoint.h>

typedef struct SandboxConfiguration
{
    int16_t startPosX = 100;
    int16_t startPosY = 100;
    int16_t startWidth = 1280;
    int16_t startHeight = 720;
    const wchar_t* Name = L"Seacrest Engine Sandbox";
} SandboxConfiguration;

// Define the function to create a game.
void CreateGame(Game* OutGame)
{
    // Sandbox has its own "config", to 
    // prevent passing in "magic" numbers. 
    SandboxConfiguration sandboxConfig;

    // Application configuration.
    OutGame->appConfig.startPosX   = sandboxConfig.startPosX;
    OutGame->appConfig.startPosY   = sandboxConfig.startPosY;
    OutGame->appConfig.startWidth  = sandboxConfig.startWidth;
    OutGame->appConfig.startHeight = sandboxConfig.startHeight;
    OutGame->appConfig.Name        = sandboxConfig.Name;
    // Renderer.
    OutGame->gfx                   = GFXAPI::Direct3D12;
    OutGame->gfxContext            = {};
    // Function pointers. 
    OutGame->Update                = SandboxUpdate;
    OutGame->Render                = SandboxRender;
    OutGame->Initialize            = SandboxInitialize;

    // Create the game state.
    OutGame->state = malloc(sizeof(SandboxState));
}
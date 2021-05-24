#include "Sandbox.h"
#include <EntryPoint.h>

// Note(rachel): Sandbox is simply an alias for a game;
// or any desirable application.

typedef struct SandboxConfiguration
{
    int16_t startPosX = 100;
    int16_t startPosY = 100;
    int16_t startWidth = 1280;
    int16_t startHeight = 720;
    const wchar_t* Name = L"Seacrest Engine Sandbox";
} SandboxConfiguration;

// Define the function to create a sandbox.
void CreateSandbox(Sandbox* OutSandbox)
{
    // Sandbox has its own "config", to 
    // prevent passing in "magic" numbers. 
    SandboxConfiguration sandboxConfig;

    // Application configuration.
    OutSandbox->appConfig.startPosX   = sandboxConfig.startPosX;
    OutSandbox->appConfig.startPosY   = sandboxConfig.startPosY;
    OutSandbox->appConfig.startWidth  = sandboxConfig.startWidth;
    OutSandbox->appConfig.startHeight = sandboxConfig.startHeight;
    OutSandbox->appConfig.Name        = sandboxConfig.Name;
    // Renderer.
    OutSandbox->gfx                   = GFXAPI::Direct3D12;
    OutSandbox->gfxContext            = {};
    // Function pointers. 
    OutSandbox->Update                = SandboxUpdate;
    OutSandbox->Render                = SandboxRender;
    OutSandbox->Initialize            = SandboxInitialize;

    // Create the sandbox state.
    OutSandbox->state = malloc(sizeof(SandboxState));
}
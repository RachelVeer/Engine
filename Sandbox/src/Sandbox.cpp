// STL.
import <cstdint>;
import <stdio.h>;
import <memory>;

// Seacrest.
import Application;
import EntryPoint;

// Note(rachel): Sandbox is simply an alias for a game;
// or any desirable application.

struct SandboxConfiguration
{
    int16_t startPosX   = 100;
    int16_t startPosY   = 100;
    int16_t startWidth  = 1280;
    int16_t startHeight = 720;
    const wchar_t* Name = L"Seacrest Engine Sandbox";
};

void Sandbox(SandboxState* OutSandbox)
{
    printf("Custom Sandbox constructor!\n");

    // Sandbox has its own "config", to 
    // prevent passing in "magic" numbers. 
    SandboxConfiguration sandboxConfig;

    // Application configuration.
    OutSandbox->appConfig.startPosX   = sandboxConfig.startPosX;
    OutSandbox->appConfig.startPosY   = sandboxConfig.startPosY;
    OutSandbox->appConfig.startWidth  = sandboxConfig.startWidth;
    OutSandbox->appConfig.startHeight = sandboxConfig.startHeight;
    OutSandbox->appConfig.Name        = sandboxConfig.Name;

    // Create the sandbox state.
    OutSandbox->state = malloc(sizeof(SandboxState));

}

void Application::CreateApplication(SandboxState* sandboxInstance)
{
    Sandbox(sandboxInstance);
}
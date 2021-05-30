#include <Engine.h>

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

class Sandbox : public Application
{
public:
    Sandbox()
    {
        printf("Default Sandbox constructor!\n");
    }
    Sandbox(SandboxState* OutSandbox)
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

        // TODO(rachel): remake renderer config. GFXAPI enum.

        // Create the sandbox state.
        OutSandbox->state = malloc(sizeof(SandboxState));
    }
    ~Sandbox()
    {}
};

Application* CreateApplication(SandboxState* sandboxInstance)
{
    return new Sandbox(sandboxInstance);
}
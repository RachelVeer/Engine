#include "Sandbox.h"
#include "Platform/Direct3D/D3D12Context.h"

Direct3D direct3d;

bool SandboxInitialize(Game* gameInstance)
{
    printf("SandboxInitialized() called!\n");
    // Temp init of D3D.
    direct3d.Init();
    return true;
}

bool SandboxUpdate(Game* gameInstance, float deltaTime)
{
    direct3d.OnUpdate();
    return true;
}

bool SandboxRender(Game* gameInstance, float deltaTime)
{
    direct3d.OnRender();
    return true;
}
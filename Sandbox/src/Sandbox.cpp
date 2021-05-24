#include "Sandbox.h"
#include "GraphicsContext.h"

Graphics gfxContext;

bool SandboxInitialize(Game* gameInstance)
{
    printf("SandboxInitialized() called!\n");
    gfxContext.Init();
    return true;
}

bool SandboxUpdate(Game* gameInstance, float deltaTime)
{
    gfxContext.Update();
    return true;
}

bool SandboxRender(Game* gameInstance, float deltaTime)
{
    gfxContext.Render();
    return true;
}
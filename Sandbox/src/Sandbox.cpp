#include "Sandbox.h"

bool SandboxInitialize(Sandbox* sandboxInstance)
{
    printf("SandboxInitialized() called!\n");
    sandboxInstance->gfxContext->Init(sandboxInstance->gfx);
    return true;
}

bool SandboxUpdate(Sandbox* sandboxInstance, float deltaTime)
{
    sandboxInstance->gfxContext->Update(sandboxInstance->gfx);
    return true;
}

bool SandboxRender(Sandbox* sandboxInstance, float deltaTime)
{
    sandboxInstance->gfxContext->Render(sandboxInstance->gfx);
    return true;
}
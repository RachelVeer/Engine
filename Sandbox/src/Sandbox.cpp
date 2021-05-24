#include "Sandbox.h"

bool SandboxInitialize(Game* gameInstance)
{
    printf("SandboxInitialized() called!\n");
    gameInstance->gfxContext->Init(gameInstance->gfx);
    return true;
}

bool SandboxUpdate(Game* gameInstance, float deltaTime)
{
    gameInstance->gfxContext->Update(gameInstance->gfx);
    return true;
}

bool SandboxRender(Game* gameInstance, float deltaTime)
{
    gameInstance->gfxContext->Render(gameInstance->gfx);
    return true;
}
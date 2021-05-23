#include "Sandbox.h"

bool SandboxInitialize(Game* gameInstance)
{
    printf("SandboxInitialized() called!\n");
    return true;
}

bool SandboxUpdate(Game* gameInstance, float deltaTime)
{
    return true;
}

bool SandboxRender(Game* gameInstance, float deltaTime)
{
    return true;
}
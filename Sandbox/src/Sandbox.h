#pragma once
#include "GameTypes.h"

typedef struct SandboxState
{
    float deltaTime;
} SandboxState;

bool SandboxInitialize(Game* gameInstance);

bool SandboxUpdate(Game* gameInstance, float deltaTime);

bool SandboxRender(Game* gameInstance, float deltaTime);
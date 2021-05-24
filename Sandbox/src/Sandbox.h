#pragma once
#include "SandboxTypes.h"

typedef struct SandboxState
{
    float deltaTime;
} SandboxState;

bool SandboxInitialize(Sandbox* sandboxInstance);

bool SandboxUpdate(Sandbox* sandboxInstance, float deltaTime);

bool SandboxRender(Sandbox* sandboxInstance, float deltaTime);
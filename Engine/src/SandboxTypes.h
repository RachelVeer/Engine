#pragma once

#include "Engine/Application.h"
#include "Engine/GraphicsContext.h"

// Represents the basic sandbox state in a sandbox.
// Called for creation by the application.

struct SandboxState
{
    // The application configuration.
    ApplicationConfiguration appConfig;

    GraphicsAPI gfxAPI;

    // Sandbox-specific sandbox state. Created and managed by the sandbox.
    void* state;
};
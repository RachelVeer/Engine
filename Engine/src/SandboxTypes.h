#pragma once

#include "Engine/Application.h"
#include "Engine/GraphicsContext.h"


// Represents the basic sandbox state in a sandbox.
// Called for creation by the application.

typedef struct Sandbox
{
    // The application configuration.
    ApplicationConfiguration appConfig;

    GFXAPI gfx;
    Graphics* gfxContext;

    // Function pointer to sandbox's initialize function.
    bool (*Initialize)(struct Sandbox* sandboxInstance);

    // Function pointer to a sandbox's update function.
    bool (*Update)(struct Sandbox* sandboxInstance, float deltaTime);

    // Function pointer to a sandbox's render function.
    bool (*Render)(struct Sandbox* sandboxInstance, float deltaTime);

    // Sandbox-specific sandbox state. Created and managed by the sandbox.
    void* state;
} Sandbox;
#pragma once

#include "Engine/Application.h"

// Represents the basic sandbox state in a sandbox.
// Called for creation by the application.

struct SandboxState
{
    // The application configuration.
    ApplicationConfiguration appConfig;

    // Sandbox-specific sandbox state. Created and managed by the sandbox.
    void* state;
};
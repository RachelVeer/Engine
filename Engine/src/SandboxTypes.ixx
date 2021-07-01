export module SandboxTypes;

import Application;

export struct SandboxState
{
    // The application configuration.
    ApplicationConfiguration appConfig;

    // Sandbox-specific sandbox state. Created and managed by the sandbox.
    void* state;
};
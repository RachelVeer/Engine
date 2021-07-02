//===================
// Application Layer.
//===================
export module Application;

// STL.
import <cstdint>;
import <thread>;

// Application configuration.
export struct ApplicationConfiguration
{
    // Note: variables could be applicable or not via platform.
    // App starting position.
    int16_t startPosX;

    // App starting position.
    int16_t startPosY;

    // App starting width.
    int16_t startWidth;

    // App starting height.
    int16_t startHeight;

    // App name, if viewable.
    const wchar_t* Name;

};

export struct SandboxState
{
    // The application configuration.
    ApplicationConfiguration appConfig;

    // Sandbox-specific sandbox state. Created and managed by the sandbox.
    void* state;
};

export namespace Application
{
    void Create(SandboxState* sandboxInstance);
    void Run();
    void Shutdown();
    void DoTime();

    // Externally defined function to create a sandbox.
    extern "C++" void CreateApplication(SandboxState * OutSandbox);
};
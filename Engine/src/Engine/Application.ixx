//===================
// Application Layer.
//===================
export module Application;

export namespace Application
{
    void Create();
    void Run();
    void Shutdown();
    void DoTime();
};

// Externally defined function to create a sandbox.
export
extern "C++" void CreateClientApp();
//*********************************************************
// Copyright (c) 2021 Rachel Veer.
// Licensed under the Apache-2.0 License.
//*********************************************************
export module EntryPoint;

// STL.
import <filesystem>;

// Seacrest.
import Application;

// While not exactly exported in a 1 to 1 module sense, Sandbox application needs to see it.
// Thus we export int main(), without the need to call it - just to see & link it. 
export 
int main(int argc, char* argv[])
{
    int minimalArgumentCount = { 1 };

    // argv[0] should be the name of the program,
    // thus there should always be at least one count present.
    if (argc >= minimalArgumentCount)
    {
        auto exeName = std::filesystem::path(argv[0]).filename();
        printf("Launched Executable: %s\n", exeName.string().c_str());
        // Request the sandbox instance from the application.
        SandboxState sandboxInstance;
        // External creation of application.
        Application::CreateApplication(&sandboxInstance);
        
        // Our actual application layer. 
        Application::Create(&sandboxInstance);
        Application::Run();
        // In any event where the application  
        // loop is broken out of - shutdown. 
        Application::Shutdown();
    }

    return 0;
}
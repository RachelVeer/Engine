//*********************************************************
// Copyright (c) 2021 Rachel Veer.
// Licensed under the Apache-2.0 License.
//*********************************************************
export module EntryPoint;

import std.filesystem;
import Application;
import Log;

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
        CoreLogger.AddLog("Launched Executable: %s\n", exeName.string().c_str());
        // Client-side creation of application.
        CreateClientApp();
        // Our engine's actual application layer. 
        Application::Create();
        Application::Run();
        Application::Shutdown();
    }

    return 0;
}
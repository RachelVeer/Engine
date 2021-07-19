// STL.
import std.core;

// Seacrest.
import Application;
import EntryPoint;
import Log;

import Layer;

// Note(rachel): Sandbox is simply an alias for a game;
// or any desirable application.

bool shown = false;
static int counter = 0;
int maxCounters = 5;

void Layer::Run()
{
    if (!shown)
    {
        CoreLogger.AddLog("Client application running too!\n");
        shown = true;
    }
    else if (counter < maxCounters)
    {
        CoreLogger.AddLog("Running proof! Called %d times in application loop.\n", counter);
        counter++;
    }
}

void Sandbox()
{
    CoreLogger.AddLog("Custom Sandbox constructor!\n");
}

void CreateClientApp() {  Sandbox(); }
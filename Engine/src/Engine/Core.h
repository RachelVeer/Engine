#pragma once
#include "Log.h"

class Core
{
public:
    Core()
    {
        // Logger.
        Log::Init();
        ENGINE_CORE_WARN("Initialized Info!\n");
        int a = 5;
        ENGINE_INFO("Hello! Var={0}\n", a);
    }
};
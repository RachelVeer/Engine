#pragma once
#include "Log.h"

class Core
{
public:
    Core()
    {
        // Logger.
        Log::Init();
        ENGINE_CORE_WARN("Initialized Info!");
        int a = 5;
        ENGINE_INFO("Hello! Var={0}", a);
    }
};
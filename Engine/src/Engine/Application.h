#pragma once

#include <thread>

#include "Platform/Platform.h"

struct Game;

// Application configuration.
typedef struct ApplicationConfiguration
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

} ApplicationConfiguration;

class Application
{
public:
    Application() = default;
    ~Application() = default;
    void Create(struct Game* gameInstance);
    void Run();
    void Shutdown();
    void DoTime();
};
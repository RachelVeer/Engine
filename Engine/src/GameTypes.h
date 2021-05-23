#pragma once

#include "Engine/Application.h"

// Represents the basic game state in a game.
// Called for creation by the application.

typedef struct Game
{
    // The application configuration.
    ApplicationConfiguration appConfig;

    // Function pointer to game's initialize function.
    bool (*Initialize)(struct Game* gameInstance);

    // Function pointer to a game's update function.
    bool (*Update)(struct Game* gameInstance, float deltaTime);

    // Function pointer to a game's render function.
    bool (*Render)(struct Game* gameInstance, float deltaTime);

    // Game-specific game state. Created and managed by the game.
    void* state;
} Game;
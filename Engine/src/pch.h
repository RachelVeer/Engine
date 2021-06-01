#pragma once

// Standard Libraries.
#include <iostream>
#include <string>
#include <filesystem>

#include <cstdint> // For fixed width integars

// Reduce size of windows header files. 
#if defined(WIN32) || defined(_WIN32) || defined(_WIN64) 
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
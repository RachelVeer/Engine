// Platform layer.
module;
#include <optional> // For unqiue message loop.
export module Platform;

namespace Platform
{
    export void Startup(
        const wchar_t* applicationName,
        int x,
        int y,
        int width,
        int height);
    export void Shutdown();
    export std::optional<int> PumpMessages();
    export const double GetAbsoluteTime();
    export const double Peek();
    // Hacky stuffs.
    export const int GetXScreenCoordinates();
    export const int GetYScreenCoordinates();
    export bool getUpArrowKey();
    export bool getDownArrowKey();
    export bool getLeftArrowKey();
    export bool getRightArrowKey();
    export void* getAdditionalPlatformData();
};
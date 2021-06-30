// Platform layer.
export module Platform;

import <cstdint>;
import <optional>; // For unqiue message loop.

namespace Platform
{
    export void Startup(
        const wchar_t* applicationName,
        int32_t x,
        int32_t y,
        int32_t width,
        int32_t height);
    export void Shutdown();
    export std::optional<int> PumpMessages();
    export const double GetAbsoluteTime();
    export const double Peek();
    // Hacky stuffs.
    export const int16_t GetXScreenCoordinates();
    export const int16_t GetYScreenCoordinates();
    export bool getUpArrowKey();
    export bool getDownArrowKey();
    export void* getAdditionalPlatformData();
};
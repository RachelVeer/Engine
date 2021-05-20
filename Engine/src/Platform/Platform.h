#pragma once

#include <cstdint>
#include <optional>

class Platform
{
public:
    void Startup(
        const wchar_t* applicationName,
        int32_t x,
        int32_t y,
        int32_t width,
        int32_t height);
    void Shutdown();
    std::optional<int> PumpMessages();
    double GetAbsoluteTime() const;
    double Peek() const;
};
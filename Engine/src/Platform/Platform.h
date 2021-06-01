#pragma once

#include <optional> // For unqiue message loop

class Platform
{
public:
    Platform() = default;
    ~Platform() = default;
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
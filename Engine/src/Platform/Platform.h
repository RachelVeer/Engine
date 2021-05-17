#pragma once

#include <cstdint>
#include <optional>
#include <memory>

class Platform
{
public:
    virtual void Startup(
        const wchar_t* applicationName,
        int32_t x,
        int32_t y,
        int32_t width,
        int32_t height) = 0;

    virtual void Shutdown() = 0;

    virtual std::optional<int> PumpMessages() = 0;

    virtual double GetAbsoluteTime() const = 0;
    virtual double Peek() const = 0;

    static std::unique_ptr<Platform> Create();
};
#pragma once

#include <cstdint>

class Platform
{
public:
    typedef struct PlatformState
    {
        void* InternalState;
    } PlatformState;
public:
    virtual void Startup(
        PlatformState* platState,
        const wchar_t* applicationName,
        int32_t x,
        int32_t y,
        int32_t width,
        int32_t height) = 0;

    virtual void Shutdown(const PlatformState* platState) = 0;

    virtual void PumpMessages(const PlatformState* platState) = 0;

    virtual double GetAbsoluteTime() const = 0;
    virtual double Peek() const = 0;

    bool IsRunning() const { return m_Running; }

    static Platform* Create();

protected:
    bool m_Running;
};
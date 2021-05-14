#pragma once

class Platform
{
public:
    typedef struct PlatformState
    {
        void* InternalState;
    } PlatformState;
public:
    void PlatformStartup(
        PlatformState* platState,
        const wchar_t* applicationName,
        int32_t x,
        int32_t y,
        int32_t width,
        int32_t height);

    void PlatformShutdown(PlatformState* platState);

    void PlatformPumpMessages(PlatformState* platState);

    double PlatformGetAbsoluteTime();

    bool platformRunning;
};
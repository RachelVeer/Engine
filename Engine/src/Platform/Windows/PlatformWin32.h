#pragma once

#include "Platform/Platform.h"

class PlatformWin32 : public Platform
{
public:
    typedef struct InternalState
    {
        HINSTANCE hInstance;
        HWND hWnd;
    } InternalState;
public:
    PlatformWin32();
    ~PlatformWin32();
    void Startup(
        PlatformState* platState,
        const wchar_t* applicationName,
        int32_t x,
        int32_t y,
        int32_t width,
        int32_t height) override;
    void Shutdown(const PlatformState* platState) override;
    void PumpMessages(const PlatformState* platState) override;
    double GetAbsoluteTime() const override;
    double Peek() const;
private:
    // This is the static callback that we register.
    static LRESULT CALLBACK s_Win32ProcessMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    // The static callback recovers the "this" pointer, 
    // and then calls this member function: 
    LRESULT Win32ProcessMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
private:
    // Clock
    static double m_ClockFrequency;
    static LARGE_INTEGER m_StartTime;
    // Win32 properties.
    const std::wstring m_wndClass = L"Engine Window Class";
    HINSTANCE m_hInstance;
};
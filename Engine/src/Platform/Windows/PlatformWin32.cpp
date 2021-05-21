//*********************************************************
// Copyright (c) 2021 Rachel Veer.
// Licensed under the Apache-2.0 License.
//*********************************************************

#include "pch.h"
#include "Platform/Platform.h"

typedef struct Clock
{
    double ClockFrequency;
    LARGE_INTEGER StartTime;
} Clock;

typedef struct Win32Props // Win32 Properties. 
{
    HWND hWnd;
    HINSTANCE hInstance;
    const std::wstring wndClass = L"Engine Window Class";
} Win32Props;

Win32Props win32props;
Clock winclock;

LRESULT CALLBACK Win32ProcessMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void Platform::Startup(
    const wchar_t* applicationName,
    int32_t x,
    int32_t y,
    int32_t width,
    int32_t height)
{
    
    // Windows 10 Creators update adds Per Monitor V2 DPI awareness context.
    // Using this awareness context allows the client area of the window 
    // to achieve 100% scaling while still allowing non-client window content to 
    // be rendered in a DPI sensitive fashion.
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    win32props.hInstance = GetModuleHandle(0);
    
    // Register the window class.
    WNDCLASSEX wc = {};
    SecureZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = Win32ProcessMessages;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = win32props.hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = win32props.wndClass.c_str();
    wc.hIconSm = NULL;

    if (!RegisterClassEx(&wc))
    {
        MessageBox(0, L"Window registration failed", L"Error", MB_ICONEXCLAMATION | MB_OK);
    };

    // Create the window.
    win32props.hWnd = CreateWindowExW(
        0,                              // Optional window styles
        win32props.wndClass.c_str(),   // Window class
        applicationName,                // Window text
        (WS_OVERLAPPED | WS_CAPTION
        | WS_SYSMENU | WS_MINIMIZEBOX), // Window style
        x, y,                           // Position
        width, height,                  // Size
        nullptr,                        // Parent window
        nullptr,                        // Menu
        win32props.hInstance,          // Instance handle
        0                               // Additional application data
    );

    // Quick and dirty error checking.
    // If failed, indicate via message box, otherwise show now created window. 
    if (win32props.hWnd == NULL)
    {
        MessageBox(NULL, L"Window creation failed", L"Error", 0);
    }
    else
    {
        // Window is hidden by default, thus we have to specify showing it.
        // Could be SW_SHOW instead of nCmdShow.
        ShowWindow(win32props.hWnd, SW_SHOW);
    }

    // Clock setup.
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    winclock.ClockFrequency = 1.0f / (double)frequency.QuadPart;
    QueryPerformanceCounter(&winclock.StartTime);
}

void Platform::Shutdown()
{
    if (win32props.hWnd)
    {
        DestroyWindow(win32props.hWnd);
        win32props.hWnd = 0;
    }
}

std::optional<int> Platform::PumpMessages()
{
    MSG msg = {};
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        // Std::optional setup inspired by ChiliTomatoNoodle.
        // Check for quit because peekmessage does not signal this via return value.
        if (msg.message == WM_QUIT)
        {
            // Return optional wrapping int (arg to PostQuitMessage is in wparam) signals quit.
            return (int)msg.wParam;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    // Return empty optional when not quitting app.
    return {};
}

double Platform::GetAbsoluteTime() const
{
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);
    return (double)currentTime.QuadPart * winclock.ClockFrequency;
}

double Platform::Peek() const
{
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);
    double elapsedTime = (currentTime.QuadPart - winclock.StartTime.QuadPart) * winclock.ClockFrequency;
    return elapsedTime;
}

LRESULT CALLBACK Win32ProcessMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_ERASEBKGND:
        {
            // Notify the OS that erasing will be handled by the application to prevent flicker.
            return 1;
        }
        //case WM_CLOSE:
        //{
        //  return 0; (We're letting application handle the closing). 
        //}
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

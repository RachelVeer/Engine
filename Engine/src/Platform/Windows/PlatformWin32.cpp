//*********************************************************
// Copyright (c) 2021 Rachel Veer.
// Licensed under the Apache-2.0 License.
//*********************************************************

#include "pch.h"

#include <chrono>
#include "PlatformWin32.h"

double PlatformWin32::m_ClockFrequency = {};
LARGE_INTEGER PlatformWin32::m_StartTime = {};

Platform* Platform::Create()
{
    return new PlatformWin32;
}

PlatformWin32::PlatformWin32()
    :m_hInstance(nullptr)
{}

PlatformWin32::~PlatformWin32()
{}

void PlatformWin32::Startup(
    PlatformState* platState,
    const wchar_t* applicationName,
    int32_t x,
    int32_t y,
    int32_t width,
    int32_t height)
{
    // Performing a "cold-cast". 
    platState->InternalState = malloc(sizeof(InternalState));
    InternalState* state = (InternalState*)platState->InternalState;

    m_hInstance = GetModuleHandle(0);
    state->hInstance = m_hInstance;
    
    // Register the window class.
    WNDCLASSEX wc = {};
    SecureZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = s_Win32ProcessMessages;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = state->hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = m_wndClass.c_str();
    wc.hIconSm = NULL;

    if (!RegisterClassEx(&wc))
    {
        MessageBox(0, L"Window registration failed", L"Error", MB_ICONEXCLAMATION | MB_OK);
    };

    // Create the window.
    HWND handle;
    handle = CreateWindowExW(
        0,                              // Optional window styles
        m_wndClass.c_str(),             // Window class
        applicationName,                // Window text
        (WS_OVERLAPPED | WS_CAPTION
        | WS_SYSMENU | WS_MINIMIZEBOX), // Window style
        CW_USEDEFAULT, CW_USEDEFAULT,   // Position 
        width, height,                  // Size
        nullptr,                        // Parent window
        nullptr,                        // Menu
        state->hInstance,               // Instance handle
        this                            // Additional application data
    );

    // Quick and dirty error checking. 
    if (handle == 0)
    {
        MessageBox(NULL, L"Window creation failed", L"Error", 0);
    }
    else
    {
        state->hWnd = handle;
    }

    // Window is hidden by default, thus we have to specify showing it.
    // Could be SW_SHOW instead of nCmdShow.
    ShowWindow(state->hWnd, SW_SHOW);

    // Clock setup.
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    m_ClockFrequency = 1.0f / (double)frequency.QuadPart;
    QueryPerformanceCounter(&m_StartTime);
}

void PlatformWin32::Shutdown(const PlatformState* platState)
{
    // Simply cold-cast to the known type.
    InternalState* state = (InternalState*)platState->InternalState;

    if (state->hWnd)
    {
        DestroyWindow(state->hWnd);
        state->hWnd = 0;
    }
}

std::optional<int> PlatformWin32::PumpMessages(const PlatformState* platState)
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

double PlatformWin32::GetAbsoluteTime() const
{
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);
    return (double)currentTime.QuadPart * m_ClockFrequency;
}

double PlatformWin32::Peek() const
{
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);
    auto elapsedTime = (currentTime.QuadPart - m_StartTime.QuadPart) * m_ClockFrequency;
    return elapsedTime;
}

LRESULT CALLBACK PlatformWin32::s_Win32ProcessMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PlatformWin32* pThis = {}; // Our "this" pointer will go here. 

    if (uMsg == WM_NCCREATE)
    {
        // Recover the "this" pointer which we passed as a parameter
        // to CreateWindow(Ex).
        LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
        pThis = static_cast<PlatformWin32*>(lpcs->lpCreateParams);

        // Put the value in a safe place for future use.
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    }
    else
    {
        // Recover "this" pointer from where our WM_NCCREATE handler
        // stashed it.
        pThis = reinterpret_cast<PlatformWin32*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (pThis)
    {
        // Now that we have recovered our "this" pointer, 
        // let the member function finish the job.
        return pThis->Win32ProcessMessages(hWnd, uMsg, wParam, lParam);
    }

    // We don't know what our "this" pointer is, 
    // so just do the default thing.
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT PlatformWin32::Win32ProcessMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

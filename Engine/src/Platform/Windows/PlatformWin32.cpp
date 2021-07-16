module;
// Platform-specific.
#include <Windows.h> 
// STL.
#include <iostream>
#include <string>
#include <optional>
// Libraries.
#include "spdlog/sinks/stdout_color_sinks.h"
#include "Engine/ImGuiLocal/ImGuiBridge.h"
module Platform;

// Seacrest modules.
import Log;
import ImGuiLocal;

struct Clock
{
    double ClockFrequency = { 0 };
    LARGE_INTEGER StartTime = { 0 };
};

struct Win32Props // Win32 Properties. 
{
    HWND hWnd = nullptr;
    HINSTANCE hInstance = nullptr;
    const std::wstring wndClass = L"Engine Window Class";
    int Width = { 0 };
    int Height = { 0 };
    POINTS pt = { 0 };
};

struct Keys
{
    bool upArrow = false;
    bool downArrow = false;
};

Win32Props win32props;
Clock winclock;
Keys simpleKeys;

static LRESULT CALLBACK Win32ProcessMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void Platform::Startup(
    const wchar_t* applicationName,
    int x,
    int y,
    int width,
    int height)
{
    // Windows 10 Creators update adds Per Monitor V2 DPI awareness context.
    // Using this awareness context allows the client area of the window 
    // to achieve 100% scaling while still allowing non-client window content to 
    // be rendered in a DPI sensitive fashion.
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    //ImGui_ImplWin32_EnableDpiAwareness();

    win32props.hInstance = GetModuleHandle(0);
    win32props.Width = width;
    win32props.Height = height;

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
    wc.hbrBackground = CreateSolidBrush(RGB(22, 22, 22));
    wc.lpszMenuName = NULL;
    wc.lpszClassName = win32props.wndClass.c_str();
    wc.hIconSm = NULL;

    if (!RegisterClassEx(&wc))
    {
        MessageBox(0, L"Window registration failed", L"Error", MB_ICONEXCLAMATION | MB_OK);
    };

    // No Maximize box, nor resizing. 
    DWORD windowStyles = (WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);

    // Store DPI for AdjustWindowRectExForDpi.
    UINT dpi = GetDpiForSystem();

    // Calculate client rectangle. 
    RECT rc = { 0 };
    rc.left = 100;
    rc.top = 100;
    rc.right = width + rc.left;
    rc.bottom = height + rc.top;

    AdjustWindowRectExForDpi(&rc, windowStyles, false, 0, dpi);

    LONG clientWidth = rc.right - rc.left;
    LONG clientHeight = rc.bottom - rc.top;

    // Create the window.
    win32props.hWnd = CreateWindowExW(
        0,                              // Optional window styles
        win32props.wndClass.c_str(),    // Window class
        applicationName,                // Window text
        windowStyles,                   // Window style
        x, y,                           // Position
        clientWidth,                    // Width
        clientHeight,                   // Height
        nullptr,                        // Parent window
        nullptr,                        // Menu
        win32props.hInstance,           // Instance handle
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
        UpdateWindow(win32props.hWnd);
    }

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(win32props.hWnd);

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
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
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

const double Platform::GetAbsoluteTime()
{
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);
    return (double)currentTime.QuadPart * winclock.ClockFrequency;
}

const double Platform::Peek()
{
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);
    double elapsedTime = (currentTime.QuadPart - winclock.StartTime.QuadPart) * winclock.ClockFrequency;
    return elapsedTime;
}

const int Platform::GetXScreenCoordinates()
{
    return win32props.pt.x;
}

const int Platform::GetYScreenCoordinates()
{
    return win32props.pt.y;
}

bool Platform::getUpArrowKey()
{
    return simpleKeys.upArrow;
}

bool Platform::getDownArrowKey()
{
    return simpleKeys.downArrow;
}

void* Platform::getAdditionalPlatformData()
{
    return win32props.hWnd;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK Win32ProcessMessages(HWND lhWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Hook ImGui into message handler, to process Win32 events (keyboard & mouse inputs, etc). 
    if (ImGui_ImplWin32_WndProcHandler(lhWnd, uMsg, wParam, lParam))
        return true;

    switch (uMsg)
    {
        //case WM_CLOSE:
        //{
        //    return 0; //(We're letting application handle the closing). 
        //}
        case WM_MOUSEMOVE:
        {
            // Capture mouse coordinates from lParam. 
            const POINTS pt = MAKEPOINTS(lParam);

            // If in client region -> log move. 
            if (pt.x >= 0 && pt.x < win32props.Width && pt.y >= 0 && pt.y < win32props.Height)
            {
                // Store values for ImGui.
                win32props.pt.x = pt.x;
                win32props.pt.y = pt.y;
                //("Mouse Coords - width: {0}, height: {1} \r", pt.x, pt.y);
            }
            return 0;
        }
        case WM_KEYDOWN:
        {
            CoreLoggerInfo("Wm_keydown");
            //printf("wm_keydown\n");
            switch (wParam)
            {
                case VK_UP:
                {
                    CoreLoggerInfo("Arrow key up!");
                    //printf("ArrowKey up!\n");
                    if (simpleKeys.downArrow)
                        simpleKeys.downArrow = false;
                    else
                        simpleKeys.upArrow = true;
                    break;
                }
                case VK_DOWN:
                {
                    CoreLoggerInfo("Arrow key down!");
                    if (simpleKeys.upArrow)
                        simpleKeys.upArrow = false;
                    else
                        simpleKeys.downArrow = true;
                    break;
                }
                default: break;
            }
            break;
        }
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(lhWnd, uMsg, wParam, lParam);
}
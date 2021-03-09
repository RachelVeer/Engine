#pragma once

#include <Windows.h>
#include <string>

// Window properties. 
struct WindowProps
{
    int Width;
    int Height;
    std::wstring Title;

    WindowProps(const std::wstring& title = L"Seacrest",
        uint32_t width = 1600,
        uint32_t height = 900)
        : Title(title), Width(width), Height(height)
    {
    }
};

class Window
{
public:
    Window();
    ~Window();

    // This is the static callback that we register.
    static LRESULT CALLBACK s_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    // The static callback recovers the "this" pointer and then
    // calls this member function. 
    LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
private:
    HINSTANCE m_hInstance;
    HWND m_hWnd;
    const std::wstring m_wndClass = L"Engine Window Class";
};
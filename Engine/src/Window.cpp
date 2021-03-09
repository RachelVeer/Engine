#include "Window.h"

Window::Window()
    : m_hInstance(nullptr), m_hWnd(nullptr)
{
    WindowProps wndProps = {};

    // Register the window class.
    WNDCLASSEX wc = {};
    SecureZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = Window::s_WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = m_hInstance;
    wc.hIcon = nullptr;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = m_wndClass.c_str();
    wc.hIconSm = nullptr;

    RegisterClassEx(&wc);

    // Create the window.
    HWND m_hWnd = CreateWindowEx(
        0,                              // Optional window styles
        m_wndClass.c_str(),             // Window class
        wndProps.Title.c_str(),         // Window text
        WS_OVERLAPPEDWINDOW,            // Window style
        CW_USEDEFAULT, CW_USEDEFAULT,   // Position 
        CW_USEDEFAULT, CW_USEDEFAULT,   // Size
        nullptr,                        // Parent window
        nullptr,                        // Menu
        m_hInstance,                    // Instance handle
        this                            // Additional application data
    );

    // Quick and dirty error checking. 
    if (m_hWnd == NULL)
    {
        MessageBox(m_hWnd, L"Error: Window creation failed", L"Error", 0);
    }

    // Window is hidden by default, thus we have to specify showing it.
    // Could be SW_SHOW instead of nCmdShow.
    if (m_hWnd != NULL)
    {
        ShowWindow(m_hWnd, SW_SHOW);
    }
}

Window::~Window()
{
}

LRESULT Window::s_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Window* pThis = {}; // Our "this" pointer will go here. 
    
    if (uMsg == WM_NCCREATE)
    {
        // Recover the "this" pointer which we passed as a parameter
        // to CreateWindow(Ex).
        LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
        pThis = static_cast<Window*>(lpcs->lpCreateParams);
        
        // Put the value in a safe place for future use.
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    }
    else
    {
        // Recover "this" pointer from where our WM_NCCREATE handler
        // stashed it.
        pThis = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (pThis)
    {
        // Now that we have recovered our "this" pointer, 
        // let the member function finish the job.
        return pThis->WndProc(hWnd, uMsg, wParam, lParam);
    }

    // We don't know what our "this" pointer is, 
    // so just do the default thing.
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT Window::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            // Generating a white background.
            HBRUSH brush = CreateSolidBrush(RGB(255, 255, 255));
            FillRect(hdc, &ps.rcPaint, brush);

            EndPaint(hWnd, &ps);
            break;
        }
        case WM_DESTROY:
        {
            DestroyWindow(hWnd);
            PostQuitMessage(0);
            return 0;
            break;
        }
        default:
        {
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
            break;
        }
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

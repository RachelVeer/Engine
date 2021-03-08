#include <Windows.h>

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance, 
    _In_opt_ HINSTANCE hPrevInstance, 
    _In_ PWSTR pCmdLine, 
    _In_ int nCmdShow) 
{
    // Wide string pointer, for lpszClassName.
    const wchar_t* WndClass = L"Engine Window Class";

    // Register the window class.
    WNDCLASSEX wc = {};
    SecureZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = nullptr;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = WndClass;
    wc.hIconSm = nullptr;

    RegisterClassEx(&wc);

    // Create the window.
    HWND hWnd = CreateWindowEx(
        0,                              // Optional window styles
        WndClass,                       // Window class
        L"Seacrest",                    // Window text
        WS_OVERLAPPEDWINDOW,            // Window style
        CW_USEDEFAULT, CW_USEDEFAULT,   // Position 
        CW_USEDEFAULT, CW_USEDEFAULT,   // Size
        nullptr,                        // Parent window
        nullptr,                        // Menu
        hInstance,                      // Instance handle
        nullptr                         // Additional application data
    );

    // Quick and dirty error checking. 
    if (hWnd == NULL)
    {
        MessageBox(hWnd, L"Error: Window creation failed", L"Error", 0);
        return 1;
    }

    // Window is hidden by default, thus we have to specify showing it.
    // Could be SW_SHOW instead of nCmdShow.
    if (hWnd != NULL)
    {
        ShowWindow(hWnd, nCmdShow);
    }

    // Message loop.
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
        }
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

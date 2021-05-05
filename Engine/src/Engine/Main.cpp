//*********************************************************
// Copyright (c) 2021 Rachel Veer.
// Licensed under the Apache-2.0 License.
//*********************************************************

#include "pch.h"

#include "Platform/Windows/Window.h"
#include "Platform/Direct3D/D3D12Context.h"

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ PWSTR pCmdLine,
    _In_ int nCmdShow)
{
    // Init window.
    Window wnd;

    // Init D3D12.
    Direct3D direct3d(wnd);

    // Message loop.
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            direct3d.OnUpdate();
            direct3d.OnRender();
        }
    }

    return (int)msg.wParam;
}
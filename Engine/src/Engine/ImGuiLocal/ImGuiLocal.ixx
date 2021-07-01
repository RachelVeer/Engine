export module ImGuiLocal;

export import <imgui/imgui.h>;
export import <imgui/backends/imgui_impl_win32.h>;
export import <imgui/backends/imgui_impl_dx12.h>;

namespace ImGuiLocal
{
    struct ClearColor
    {
        float r, g, b, a;
    };

    export void Init();
    export void BeginFrame();
    export void DemoWindows(float& color, bool& show);
    export void EndFrame();
    export void Shutdown();
};
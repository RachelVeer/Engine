module ImGuiLocal;

import <imgui/imgui.h>;
import <imgui/backends/imgui_impl_win32.h>;
import <imgui/backends/imgui_impl_dx12.h>;

void ImGuiLocal::Init()
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = 1920.0f;
    io.DisplaySize.y = 1080.0f;
    //io.FontGlobalScale = 1.2f;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // WARNING: this font file must be copiedto"Sandbox"directory. 
    // It's under vendor/imgui/misc/fonts.
    io.Fonts->AddFontFromFileTTF("../../../Engine/vendor/imgui/misc/fonts/Roboto-Medium.ttf", 16.0f);

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
}

void ImGuiLocal::BeginFrame()
{
    // Start the Dear ImGui frame
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLocal::DemoWindows(float& color, bool& show)
{
    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show)
        ImGui::ShowDemoWindow(&show);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show);      // Edit bools storing our window open/close state

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", &color); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }
}

void ImGuiLocal::EndFrame()
{
    ImGui::Render();
}

void ImGuiLocal::Shutdown()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}
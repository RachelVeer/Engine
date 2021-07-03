module; 
#include "D3D12Bridge.h"
export module D3D12Context;

// STL
import <cstdint>;
// . . .
import ImGuiLocal;

export 
{
    // Defining the area we draw to.
    struct Surface
    {
        int32_t width, height;
    };

    Surface surface;

    // Imgui helper with clearing surface color.
    ImVec4 clear_color;

    // Defining geometry and additional data for graphics/shader pipleine.
    struct Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT4 color;
        DirectX::XMFLOAT2 uv;
    };

    // Detailing a constant occuring within our scene. 
    struct SceneConstantBuffer
    {
        DirectX::XMFLOAT4 offset;
        DirectX::XMFLOAT4 cbcolor;
        float padding[56]; // Padding so the constant buffer is 256-byte aligned. 
    };
    static_assert((sizeof(SceneConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

    struct Lerp
    {
        float mixColor = 0.2f;
        float padding[63]; // Padding so the constant buffer is 256-byte aligned. 
    };
    static_assert((sizeof(Lerp) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

    struct CBVResources // Not so much literally accessing D3D constant buffers, but affecting it with these parameters. 
    {
        bool forward = true;
        const float translationSpeed = 0.005f;
        // "bounds" relative to uniformed space within DirectX, not our screens.  
        const float offsetBounds = 1.25f; // Right edge
        const float negoffsetBounds = -0.55f; // Left edge
    };

    CBVResources cbvParams;

    // Relevant D3D parameters.
    const uint32_t g_FrameCount = 2;
    float g_aspectRatio = { 0 };

    // Pipeline objects.
    CD3DX12_VIEWPORT g_Viewport;
    CD3DX12_RECT g_ScissorRect;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> g_SwapChain;
    Microsoft::WRL::ComPtr<ID3D12Device4> g_Device;
    Microsoft::WRL::ComPtr<IDXGIFactory4> g_Factory;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> g_CommandQueue;
    Microsoft::WRL::ComPtr<ID3D12Resource> g_RenderTargets[g_FrameCount];
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_rtvHeap; // Render Target View Heap.
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_srvHeap; // Our main Descriptor Heap.
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_samplerHeap;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> g_CommandAllocator;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> g_RootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> g_PipelineState;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> g_PipelineState2;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> g_CommandList;
    uint32_t g_rtvDescriptorSize;
    uint32_t g_srvDescriptorSize;
    uint32_t g_samplerDescriptorSize;

    D3D12_CPU_DESCRIPTOR_HANDLE  g_mainRenderTargetDescriptor[g_FrameCount] = {};

    // App resources 
    Microsoft::WRL::ComPtr<ID3D12Resource> g_VertexBuffer, g_IndexBuffer, g_ConstantBuffer, g_LerpConstantBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> g_VertexBuffer2, g_IndexBuffer2;
    D3D12_VERTEX_BUFFER_VIEW g_VertexBufferView;
    D3D12_VERTEX_BUFFER_VIEW g_VertexBufferView2;
    D3D12_INDEX_BUFFER_VIEW  g_IndexBufferView;
    D3D12_INDEX_BUFFER_VIEW  g_IndexBufferView2;
    SceneConstantBuffer g_constantBufferData;
    Lerp g_LerpCBData;
    UINT8* g_pCbvDataBegin;
    UINT8* g_pLerpCbvDataBegin;
    Microsoft::WRL::ComPtr<ID3D12Resource> g_Texture, g_Texture2;

    // For our textures.
    const uint32_t TextureWidth = 512;
    const uint32_t TextureHeight = 512;


    // Synchronization objects.
    uint32_t g_FrameIndex;
    HANDLE g_FenceEvent;
    Microsoft::WRL::ComPtr<ID3D12Fence> g_Fence;
    uint64_t g_FenceValue = 0;

    HWND g_StoredHwnd;

    // Forward declarations. 
    void CreateRenderTarget();
    void CleanupRenderTarget();
    void LoadPipeline();
    void LoadAssets();
    void GetHardwareAdapter(IDXGIFactory4* pFactory, IDXGIAdapter1** ppAdapter,
        bool requestHighPerformanceAdapter);
    void WaitForPreviousFrame();
    void PopulateCommandList();

    void D3D12ContextMod()
    {
        printf("Printing from D3D12Context module.\n");
    }
}
//*********************************************************
// Copyright (c) 2021 Rachel Veer.
// Licensed under the Apache-2.0 License.
//*********************************************************
#include "pch.h"
#include "Engine/GraphicsContext.h"
#include "Platform/Platform.h"
#include "Engine/Log.h"

#include "Engine/ImGuiLocal/ImGuiLocal.h"

// DirectX specific code & libraries will only link/compile
// relative to the graphics layer if it's actually defined.

#if defined(ENGINE_GRAPHICS_DIRECTX12)

// Com & D3D headers.
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <D3Dcompiler.h>
#include <commapi.h>

// Linking necessary libraries.
#pragma comment (lib, "D3d12.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3dcompiler.lib")
#pragma comment (lib, "dxguid.lib") // For DX Modules functionality. 

// D3D12 Helper functions.
#include "Platform/Direct3D/Utils/d3dx12.h" 
#include "Platform/Direct3D/Utils/DXHelper.h"

// DirectX 12 Toolkit functionality.
#include <ScreenGrab/ScreenGrab12.h>
#include <WICTextureLoader/WICTextureLoader12.h>

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

struct CBVResources // Not so much literally accessing D3D constant buffers, but affecting it with these parameters. 
{
    bool forward = true;
    const float translationSpeed = 0.005f;
    // "bounds" relative to uniformed space within DirectX, not our screens.  
    const float offsetBounds    =  1.25f; // Right edge
    const float negoffsetBounds = -0.55f; // Left edge
};

CBVResources cbvParams;

// Relevant D3D parameters.
static const uint32_t g_FrameCount = 2;
float g_aspectRatio = { 0 };

// Pipeline objects.
CD3DX12_VIEWPORT g_Viewport;
CD3DX12_RECT g_ScissorRect;
Microsoft::WRL::ComPtr<IDXGISwapChain3> g_SwapChain;
Microsoft::WRL::ComPtr<ID3D12Device4> g_Device;
Microsoft::WRL::ComPtr<IDXGIFactory4> g_Factory;
Microsoft::WRL::ComPtr<ID3D12CommandQueue> g_CommandQueue;
Microsoft::WRL::ComPtr<ID3D12Resource> g_RenderTargets[g_FrameCount];
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_rtvHeap;
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_pd3dSrvDescHeap; // This heap belongs to Imgui.
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_srvHeap;
Microsoft::WRL::ComPtr<ID3D12CommandAllocator> g_CommandAllocator;
Microsoft::WRL::ComPtr<ID3D12RootSignature> g_RootSignature;
Microsoft::WRL::ComPtr<ID3D12PipelineState> g_PipelineState;
Microsoft::WRL::ComPtr<ID3D12PipelineState> g_PipelineState2;
Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> g_CommandList;
uint32_t g_rtvDescriptorSize;

static D3D12_CPU_DESCRIPTOR_HANDLE  g_mainRenderTargetDescriptor[g_FrameCount] = {};

// App resources 
Microsoft::WRL::ComPtr<ID3D12Resource> g_VertexBuffer, g_IndexBuffer, g_ConstantBuffer;
Microsoft::WRL::ComPtr<ID3D12Resource> g_VertexBuffer2, g_IndexBuffer2;
D3D12_VERTEX_BUFFER_VIEW g_VertexBufferView;
D3D12_VERTEX_BUFFER_VIEW g_VertexBufferView2;
D3D12_INDEX_BUFFER_VIEW  g_IndexBufferView;
D3D12_INDEX_BUFFER_VIEW  g_IndexBufferView2;
SceneConstantBuffer g_constantBufferData;
UINT8* g_pCbvDataBegin;
Microsoft::WRL::ComPtr<ID3D12Resource> g_Texture, g_Texture2;

// For mockup texture
static const uint32_t TextureWidth = 512;
static const uint32_t TextureHeight = 512;
static const uint32_t TexturePixelSize = 4; // The number of bytes used to represent a pixel in the texture.


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
void GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter,
    bool requestHighPerformanceAdapter);
void WaitForPreviousFrame();
void PopulateCommandList();
std::vector<uint8_t> GenerateTextureData();

// We only namespace after variables to make it obvious where 
// ComPtr truly originated from. Plus general explicitness on 
// variable creation. 
using namespace Microsoft::WRL;
using namespace DirectX;

void Graphics::Init(int32_t width, int32_t height)
{
    ENGINE_CORE_DEBUG("Current Graphics API: Direct3D12.");
    
    // Storing incoming/external data.
    g_StoredHwnd = GetActiveWindow();
    surface.width = width;
    surface.height = height;

    // Now preparing data for pipeline.
    g_aspectRatio = static_cast<float>(surface.width) / static_cast<float>(surface.height);

    g_Viewport.Width     = (float)surface.width;
    g_Viewport.Height    = (float)surface.height;
    g_ScissorRect.right  = surface.width;
    g_ScissorRect.bottom = surface.height;

    // Then we can call upon the actual api. 
    LoadPipeline();
    LoadAssets();
    ImGui_ImplDX12_Init(g_Device.Get(), g_FrameCount,
        DXGI_FORMAT_R8G8B8A8_UNORM, g_pd3dSrvDescHeap.Get(),
        g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
        g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());
}

// Update frame-based values.
void Graphics::Update(ClearColor& color)
{
    // By default it moves forward, thus once we reach offsetBounds - set it false.
    if (g_constantBufferData.offset.x > cbvParams.offsetBounds) { cbvParams.forward = false; }
    // And once it reaches negative bounds, it can move forward again.
    if (g_constantBufferData.offset.x < cbvParams.negoffsetBounds) { cbvParams.forward = true; }

    if (cbvParams.forward)
    {
        g_constantBufferData.offset.x += cbvParams.translationSpeed;
    }

    if (!cbvParams.forward)
    {
        g_constantBufferData.offset.x -= cbvParams.translationSpeed;
    }

    g_constantBufferData.cbcolor.y = color.g;

    memcpy(g_pCbvDataBegin, &g_constantBufferData, sizeof(g_constantBufferData));
}

void Graphics::Render(ClearColor& color)
{
    clear_color = ImVec4(color.r, color.g, color.b, color.a);

    // Record all the commands we need to render the scene into the command list.
    PopulateCommandList();

    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { g_CommandList.Get() };
    g_CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Present the frame.
    ThrowIfFailed(g_SwapChain->Present(1, 0));

    WaitForPreviousFrame();
}

void Graphics::Screenshot()
{
    UINT backBufferIdx = g_SwapChain->GetCurrentBackBufferIndex();
    SaveDDSTextureToFile(g_CommandQueue.Get(), g_RenderTargets[backBufferIdx].Get(), L"test.dds", D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_PRESENT);
}

void Graphics::Shutdown()
{
    CleanupRenderTarget();
}

void LoadPipeline()
{
    // Enable the D3D12 debug layer. 
    // This must be called before the D3D12 device is created. 
    // Otherwise the D3D12 device will be removed. 
    {
        Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
        }
    }

    // Create the device. 
    {
        // Don't mix the use of DXGI 1.0 (IDXGIFactory) and DXGI 1.1 (IDXGIFactory1) in an application.
        ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&g_Factory)));

        Microsoft::WRL::ComPtr<IDXGIAdapter1> hardwareAdapter;
        GetHardwareAdapter(g_Factory.Get(), &hardwareAdapter, true);

        ThrowIfFailed(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&g_Device)));
    }

    // Create the command queue. 
    {
        D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
        cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

        ThrowIfFailed(g_Device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&g_CommandQueue)));
    }

    // Create the swap chain
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = 0;
    swapChainDesc.Height = 0;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = false;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = g_FrameCount;
    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE; // Don't worry about transparency (for now).
    swapChainDesc.Flags = 0;

    Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(g_Factory->CreateSwapChainForHwnd(
        g_CommandQueue.Get(), // In D3D12, it points to a direct Command Queue.
        g_StoredHwnd,
        &swapChainDesc,
        nullptr,              // Not worrying about fullscreen capabilities as of now. 
        nullptr,
        &swapChain
    ));

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(g_Factory->MakeWindowAssociation(g_StoredHwnd, DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain.As(&g_SwapChain));
    g_FrameIndex = g_SwapChain->GetCurrentBackBufferIndex();

    // Create a render target view (RTV) descriptor heap.
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = g_FrameCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(g_Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&g_rtvHeap)));

        g_rtvDescriptorSize = g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    // Create frame resources (RTV for each frame).
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(g_rtvHeap->GetCPUDescriptorHandleForHeapStart());

        // Create a RTV for each frame.
        for (uint32_t n = 0; n < g_FrameCount; n++)
        {
            g_mainRenderTargetDescriptor[n] = rtvHandle;
            rtvHandle.ptr += g_rtvDescriptorSize;
        }
    }

    {
        // A descriptor heap relevant to imgui funcitonality. 
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 1;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        (g_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)));
    }

    { 
        // Describe and create a shader resource view (SRV) heap for the texture.
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
        srvHeapDesc.NumDescriptors = 2;
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        ThrowIfFailed(g_Device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&g_srvHeap)));
    }

    // Create frame resources.
    CreateRenderTarget();
    
    // Create command allocator.
    {
        ThrowIfFailed(g_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_CommandAllocator)));
    }
}

void LoadAssets()
{
    // Create a root signature of a descriptor table with a single CBV. 
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

        // This is the highest version the app supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        if (FAILED(g_Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        // We only need one range for the shader resource view.
        // Whereas two rootparameters are used to not only account for the SRV,
        // but to describe our constant buffer as well - or RootConstant in this case. 
        CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
        CD3DX12_ROOT_PARAMETER1 rootParameters[2];

        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
        // "3" values, reflecting our constant buffer (yes, even including the padding). 
        rootParameters[0].InitAsConstants(3, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        // Descriptor table, which will point to our SRV descriptor within our original SRV descriptor heap. 
        rootParameters[1].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
        //rootParameters[2].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.MipLODBias = 0;
        sampler.MaxAnisotropy = 0;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler.MinLOD = 0.0f;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0;
        sampler.RegisterSpace = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // Allow input layout and deny uneccessary access to certain pipeline stages.
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, rootSignatureFlags);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
        ThrowIfFailed(g_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&g_RootSignature)));
    }

    // Create the pipeline state, which includes compiling and loading shaders.
    {
        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif

        ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"src/assets/shaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
        ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"src/assets/shaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

        // Define the vertex input layout.
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = g_RootSignature.Get();
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;
        ThrowIfFailed(g_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_PipelineState)));
       
        /*
        // Second set of shaders 
        ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"src/assets/shaders2.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
        ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"src/assets/shaders2.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

        // The previous desc bleeds through, the only thing we alter are the shaders set.
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
        
        // Create a second pso from that. 
        ThrowIfFailed(g_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_PipelineState2)));
        */
    }

    // Create command list. (version "1" automatically closes itself - one less step). 
    g_Device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&g_CommandList));
    
    // Create the vertex buffer.
    {
        // Define the geometry for a triangle.
        Vertex triangleVertices[] =
        {
            // Clockwise.
            { { -0.25f,  0.25f * g_aspectRatio, 0.0f}, { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } }, // top left
            { {  0.25f, -0.25f * g_aspectRatio, 0.0f}, { 0.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } }, // bottom right
            { { -0.25f, -0.25f * g_aspectRatio, 0.0f}, { 0.0f, 0.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } }, // bottom left
            { {  0.25f,  0.25f * g_aspectRatio, 0.0f}, { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } }, // top right
        };

        const uint32_t vertexBufferSize = sizeof(triangleVertices);

        // Note: using upload heaps to transfer static data like vert buffers is not 
        // recommended. Every time the GPU needs it, the upload heap will be marshalled 
        // over. Please read up on Default Heap usage. An upload heap is used here for 
        // code simplicity and because there are very few verts to actually transfer.
        ThrowIfFailed(g_Device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&g_VertexBuffer)));

        // Copy the triangle data to the vertex buffer.
        UINT8* pVertexDataBegin = { 0 };
        CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(g_VertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
        g_VertexBuffer->Unmap(0, nullptr);

        // Initialize the vertex buffer view.
        g_VertexBufferView.BufferLocation = g_VertexBuffer->GetGPUVirtualAddress();
        g_VertexBufferView.StrideInBytes = sizeof(Vertex);
        g_VertexBufferView.SizeInBytes = vertexBufferSize;
    }

    // Create the index buffer
    {
        // Define indices 
        int16_t Indices[] =
        {
            0, 1, 2,
            0, 3, 1
        };

        const uint32_t indexBufferSize = sizeof(Indices);

        ThrowIfFailed(g_Device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&g_IndexBuffer)
        ));

        // Copy the triangle data to index buffer.
        UINT8* pIndexDataBegin = { 0 };
        CD3DX12_RANGE readRange(0, 0);
        ThrowIfFailed(g_IndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
        memcpy(pIndexDataBegin, Indices, sizeof(Indices));
        g_IndexBuffer->Unmap(0, nullptr);

        // Initialize index buffer view.
        g_IndexBufferView.BufferLocation = g_IndexBuffer->GetGPUVirtualAddress();
        g_IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
        g_IndexBufferView.SizeInBytes = indexBufferSize;
    }

    /*
    // SECOND ENITRELY SEPARATE TRIANGLE // 
    // ================================= //
    // Create the vertex buffer.
    {
        // At the end of the day, we're simply defining points in space.
        // Which usually come in two, (A, B) -> [0], [1].
        {
            // Original triangle position
            float top[] = { 0.30f, 0.25f };
            float bottomRight[] = { 0.55f, -0.25f };
            float bottomLeft[] = { 0.05f, -0.25f };
        }
        // Upside down triangle 
        float top[] = { -0.30f, -0.25f };
        float bottomRight[] = { -0.55f, 0.25f };
        float bottomLeft[] = { -0.05f, 0.25f };
        // Define the geometry for a triangle.
        Vertex triangleVertices[] =
        {
            // Clockwise.
            { {  top[0],  top[1] * g_aspectRatio, 0.0f}, { 0.8f, 0.0f, 0.8f, 1.0f } },
            { {  bottomRight[0], bottomRight[1] * g_aspectRatio, 0.0f}, { 0.0f, 0.8f, 0.8f, 1.0f } },
            { {  bottomLeft[0],  bottomLeft[1] * g_aspectRatio, 0.0f}, { 0.8f, 0.8f, 0.0f, 1.0f } }, 
        };

        const uint32_t vertexBufferSize = sizeof(triangleVertices);

        // Note: using upload heaps to transfer static data like vert buffers is not 
        // recommended. Every time the GPU needs it, the upload heap will be marshalled 
        // over. Please read up on Default Heap usage. An upload heap is used here for 
        // code simplicity and because there are very few verts to actually transfer.
        ThrowIfFailed(g_Device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&g_VertexBuffer2)));

        // Copy the triangle data to the vertex buffer.
        UINT8* pVertexDataBegin = { 0 };
        CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(g_VertexBuffer2->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
        g_VertexBuffer2->Unmap(0, nullptr);

        // Initialize the vertex buffer view.
        g_VertexBufferView2.BufferLocation = g_VertexBuffer2->GetGPUVirtualAddress();
        g_VertexBufferView2.StrideInBytes = sizeof(Vertex);
        g_VertexBufferView2.SizeInBytes = vertexBufferSize;
    }

    // Create the index buffer
    {
        // Define indices 
        int16_t Indices[] =
        {
            0, 1, 2,
        };

        const uint32_t indexBufferSize = sizeof(Indices);

        ThrowIfFailed(g_Device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&g_IndexBuffer2)
        ));

        // Copy the triangle data to index buffer.
        UINT8* pIndexDataBegin = { 0 };
        CD3DX12_RANGE readRange(0, 0);
        ThrowIfFailed(g_IndexBuffer2->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
        memcpy(pIndexDataBegin, Indices, sizeof(Indices));
        g_IndexBuffer2->Unmap(0, nullptr);

        // Initialize index buffer view.
        g_IndexBufferView2.BufferLocation = g_IndexBuffer->GetGPUVirtualAddress();
        g_IndexBufferView2.Format = DXGI_FORMAT_R16_UINT;
        g_IndexBufferView2.SizeInBytes = indexBufferSize;
    } */

    // Create the constant buffer.
    {
        const UINT constantBufferSize = sizeof(SceneConstantBuffer); // CB size is required to be 256-byte aligned.
        ThrowIfFailed(g_Device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), 
             D3D12_HEAP_FLAG_NONE,
             &CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize),
             D3D12_RESOURCE_STATE_GENERIC_READ, 
             nullptr, 
             IID_PPV_ARGS(&g_ConstantBuffer)));

        // NOTE: No constant buffer view is being created here anymore - 
        // as the rootsignature handles it being 'set' as a rootconstant now.

        // Map and intialize the constant buffer. We don't unmap this until the
        // app closes. Keeping things mapped for the lifetime of the resource is okay.
        CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource of the CPU.
        ThrowIfFailed(g_ConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&g_pCbvDataBegin)));
        memcpy(g_pCbvDataBegin, &g_constantBufferData, sizeof(g_constantBufferData));
    }

    // Note: ComPtr's are CPU objects but this resource needs to stay in scope until
    // the command list that references it has finished executing on the GPU.
    // We will flush the GPU at the end of this method to ensure the resource is not
    // prematurely destroyed.
    ComPtr<ID3D12Resource> textureUploadHeap;

    // Create the texture.
    {
        // Describe and create a Texture2D.
        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.MipLevels = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.Width = TextureWidth;
        textureDesc.Height = TextureHeight;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        textureDesc.DepthOrArraySize = 1;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

        ThrowIfFailed(g_Device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&g_Texture)
        ));
        
        // Copy data to the intermediate upload heap and then schedule a copy
        // from the upload heap to the Texture2D.
        std::unique_ptr<uint8_t[]> decodedData;
        D3D12_SUBRESOURCE_DATA subresouce;
        LoadWICTextureFromFile(g_Device.Get(), L"container.jpg", &g_Texture, decodedData, subresouce);
        

        const UINT64 uploadBufferSize = GetRequiredIntermediateSize(g_Texture.Get(), 0, 1);

        // Create the GPU upload buffer.
        ThrowIfFailed(g_Device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&textureUploadHeap)
        ));

        // CommandList1 closes itself after creation, so we reset commandlist to call UpdateSubresources.
        g_CommandList->Reset(g_CommandAllocator.Get(), g_PipelineState.Get());
        // Copy upload contents to default heap.
        UpdateSubresources(g_CommandList.Get(), g_Texture.Get(), textureUploadHeap.Get(), 0, 0, 1, &subresouce);
        // Transistion the texture default heap to pixel shader resource. 
        g_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(g_Texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
        // Close command list once more.
        g_CommandList->Close();
        // Execute it.
        ID3D12CommandList* ppCommandLists[] = { g_CommandList.Get() };
        g_CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        // Describe and create a SRV for the texture.
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = textureDesc.Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        g_Device->CreateShaderResourceView(g_Texture.Get(), &srvDesc, g_srvHeap->GetCPUDescriptorHandleForHeapStart());
    }

    // Release previous temp resource to reuse again.
    ComPtr<ID3D12Resource> textureUploadHeap2;

    // Create the second texture.
    {
        // Describe and create a Texture2D.
        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.MipLevels = 1;
        textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        textureDesc.Width = TextureWidth;
        textureDesc.Height = TextureHeight;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        textureDesc.DepthOrArraySize = 1;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

        ThrowIfFailed(g_Device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&g_Texture2)
        ));

        // Copy data to the intermediate upload heap and then schedule a copy
        // from the upload heap to the Texture2D.
        std::unique_ptr<uint8_t[]> decodedData;
        D3D12_SUBRESOURCE_DATA subresouce;
        LoadWICTextureFromFile(g_Device.Get(), L"awesomeface.png", &g_Texture2, decodedData, subresouce);


        const UINT64 uploadBufferSize = GetRequiredIntermediateSize(g_Texture2.Get(), 0, 1);

        // Create the GPU upload buffer.
        ThrowIfFailed(g_Device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&textureUploadHeap2)
        ));

        // CommandList1 closes itself after creation, so we reset commandlist to call UpdateSubresources.
        g_CommandList->Reset(g_CommandAllocator.Get(), g_PipelineState.Get());
        // Copy upload contents to default heap.
        UpdateSubresources(g_CommandList.Get(), g_Texture2.Get(), textureUploadHeap2.Get(), 0, 0, 1, &subresouce);
        // Transistion the texture default heap to pixel shader resource. 
        g_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(g_Texture2.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
        // Close command list once more.
        g_CommandList->Close();
        // Execute it.
        ID3D12CommandList* ppCommandLists[] = { g_CommandList.Get() };
        g_CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        // Describe and create a SRV for the texture.
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = textureDesc.Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        g_Device->CreateShaderResourceView(g_Texture2.Get(), &srvDesc, g_srvHeap->GetCPUDescriptorHandleForHeapStart());
    }

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        ThrowIfFailed(g_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_Fence)));
        g_FenceValue = 1;

        // Create an event handle to use for frame synchronization.
        g_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (g_FenceEvent == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        // Wait for the command list to execute; we are reusing the same command
        // list in our main loop but for now, we just want to wait for setup to
        // complete before continuing. 
        WaitForPreviousFrame();
    }
}

void PopulateCommandList()
{
    // Command list allocators can only be reset when the associated
    // command lists have finished execution on the GPU; apps should use
    // fences to determine GPU execution progress.
    ThrowIfFailed(g_CommandAllocator->Reset());

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(g_rtvHeap->GetCPUDescriptorHandleForHeapStart(), g_FrameIndex, g_rtvDescriptorSize);
  
    UINT backBufferIdx = g_SwapChain->GetCurrentBackBufferIndex();

    // Preparing resource barrier, alternatively, there's -> CD3DX12_RESOURCE_BARRIER::Transition
    // for a more streamlined process. 
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = g_RenderTargets[backBufferIdx].Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    g_CommandList->Reset(g_CommandAllocator.Get(), g_PipelineState.Get());
    g_CommandList->ResourceBarrier(1, &barrier);

    // Set necessary state.
    g_CommandList->SetGraphicsRootSignature(g_RootSignature.Get());

    ID3D12DescriptorHeap* ppHeaps[] = { g_srvHeap.Get() };
    g_CommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    // Again, 3 values to reflect our contant buffer members (including padding).
    g_CommandList->SetGraphicsRoot32BitConstants(0, 3, g_pCbvDataBegin, 0);
    g_CommandList->SetGraphicsRootDescriptorTable(1, g_srvHeap->GetGPUDescriptorHandleForHeapStart());
    //g_CommandList->SetGraphicsRootDescriptorTable(2, g_srvHeap->GetGPUDescriptorHandleForHeapStart());
    g_CommandList->RSSetViewports(1, &g_Viewport);
    g_CommandList->RSSetScissorRects(1, &g_ScissorRect);

    // Record commands.
    // Clear color with alpha blending.
    const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };

    // Render target view.
    g_CommandList->ClearRenderTargetView(g_mainRenderTargetDescriptor[backBufferIdx], clear_color_with_alpha, 0, NULL);
    g_CommandList->OMSetRenderTargets(1, &g_mainRenderTargetDescriptor[backBufferIdx], FALSE, NULL);
    
    g_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // First triangle.
    g_CommandList->IASetVertexBuffers(0, 1, &g_VertexBufferView);
    g_CommandList->IASetIndexBuffer(&g_IndexBufferView);
    g_CommandList->DrawIndexedInstanced(6, 1, 0, 0, 0); 

    // Second pipeline state
    // The command list resets itself above, with the original pipeline state. 
    // Thus we don't need to set pso "1" after the second triangle, or call it before the
    // first triangle. 
    //g_CommandList->SetPipelineState(g_PipelineState2.Get());

    // Second triangle.
    //g_CommandList->IASetVertexBuffers(0, 1, &g_VertexBufferView2);
    //g_CommandList->IASetIndexBuffer(&g_IndexBufferView2);
    //g_CommandList->DrawIndexedInstanced(3, 1, 0, 0, 0);
    
    // Related to Imgui.
    g_CommandList->SetDescriptorHeaps(1, g_pd3dSrvDescHeap.GetAddressOf());
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_CommandList.Get());
    
    // Prepare frame for presenting/drawing. 
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    g_CommandList->ResourceBarrier(1, &barrier);

    ThrowIfFailed(g_CommandList->Close());
}

void WaitForPreviousFrame()
{
    // Warning: Waiting for frame completion is not best practice.
    // This is just for simplicity, I will implement a more elegant
    // example from the D3D12 samples repository. 

    // Signal and increment the fence value.
    const uint64_t fence = g_FenceValue;
    ThrowIfFailed(g_CommandQueue->Signal(g_Fence.Get(), fence));
    g_FenceValue++;

    // Wait until the previous frame is finished.
    if (g_Fence->GetCompletedValue() < fence)
    {
        ThrowIfFailed(g_Fence->SetEventOnCompletion(fence, g_FenceEvent));
        WaitForSingleObject(g_FenceEvent, INFINITE);
    }

    g_FrameIndex = g_SwapChain->GetCurrentBackBufferIndex();
}

void CreateRenderTarget()
{
    for (UINT i = 0; i < g_FrameCount; i++)
    {
        ID3D12Resource* pBackBuffer = NULL;
        g_SwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
        g_Device->CreateRenderTargetView(pBackBuffer, NULL, g_mainRenderTargetDescriptor[i]);
        g_RenderTargets[i] = pBackBuffer;
    }
}

void CleanupRenderTarget()
{
    WaitForPreviousFrame();
    for (UINT i = 0; i < g_FrameCount; i++)
        if (g_RenderTargets[i]) { g_RenderTargets[i]->Release(); g_RenderTargets[i] = nullptr; }
}

// Check adapter support.
void GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter,
    bool requestHighPerformanceAdapter)
{
    *ppAdapter = nullptr;

    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;

    Microsoft::WRL::ComPtr<IDXGIFactory6> factory6;
    if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
    {
        for (
            UINT adapterIndex = 0;
            DXGI_ERROR_NOT_FOUND != factory6->EnumAdapterByGpuPreference(
                adapterIndex,
                requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
                IID_PPV_ARGS(&adapter));
            ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                // If you want a software adapter, pass in "/warp" on the command line.
                continue;
            }

            // Check to see whether the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }
    }
    else
    {
        for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                // If you want a software adapter, pass in "/warp" on the command line.
                continue;
            }

            // Check to see whether the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }
    }

    *ppAdapter = adapter.Detach();
}

// Generate a simple black and white checkerboard texture.
std::vector<uint8_t> GenerateTextureData()
{
    const uint32_t rowPitch = TextureWidth * TexturePixelSize;
    const uint32_t cellPitch = rowPitch >> 3;        // The width of a cell in the checkboard texture.
    const uint32_t cellHeight = TextureWidth >> 3;    // The height of a cell in the checkerboard texture.
    const uint32_t textureSize = rowPitch * TextureHeight;

    std::vector<uint8_t> data(textureSize);
    uint8_t* pData = &data[0];

    for (uint32_t n = 0; n < textureSize; n += TexturePixelSize)
    {
        uint32_t x = n % rowPitch;
        uint32_t y = n / rowPitch;
        uint32_t i = x / cellPitch;
        uint32_t j = y / cellHeight;

        if (i % 2 == j % 2)
        {
            pData[n] = 0x00;        // R
            pData[n + 1] = 0x00;    // G
            pData[n + 2] = 0x00;    // B
            pData[n + 3] = 0xff;    // A
        }
        else
        {
            pData[n] = 0xff;        // R
            pData[n + 1] = 0xff;    // G
            pData[n + 2] = 0xff;    // B
            pData[n + 3] = 0xff;    // A
        }
    }

    return data;
}

#endif
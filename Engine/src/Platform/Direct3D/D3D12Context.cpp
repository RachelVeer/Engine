//*********************************************************
// Copyright (c) 2021 Rachel Veer.
// Licensed under the Apache-2.0 License.
//*********************************************************
#include "pch.h"
#include "Engine/Core.h"
#include "Engine/GraphicsContext.h"
#include "Platform/Platform.h"

#include "Engine/ImGuiLocal/ImGuiLocal.h"

// DirectX specific code & libraries will only link/compile
// relative to the graphics layer if it's actually defined.

#if defined(D3D) || defined(D3D12)

// Linking necessary libraries.
#pragma comment (lib, "D3d12.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3dcompiler.lib")

// Com & D3D headers.
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <D3Dcompiler.h>
#include <commapi.h>

// D3D12 Helper functions.
#include "Platform/Direct3D/Utils/d3dx12.h" 
#include "Platform/Direct3D/Utils/DXHelper.h"

static const uint32_t g_FrameCount = 2;

ImVec4 clear_color; 
struct Vertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 color;
};

struct Surface
{
    int32_t width, height;
};

Surface surface;

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
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_pd3dSrvDescHeap;
Microsoft::WRL::ComPtr<ID3D12CommandAllocator> g_CommandAllocator;
Microsoft::WRL::ComPtr<ID3D12RootSignature> g_RootSignature;
Microsoft::WRL::ComPtr<ID3D12PipelineState> g_PipelineState;
Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> g_CommandList;
uint32_t g_rtvDescriptorSize;

static D3D12_CPU_DESCRIPTOR_HANDLE  g_mainRenderTargetDescriptor[g_FrameCount] = {};

// App resources 
Microsoft::WRL::ComPtr<ID3D12Resource> g_VertexBuffer, g_IndexBuffer;
D3D12_VERTEX_BUFFER_VIEW g_VertexBufferView;
D3D12_INDEX_BUFFER_VIEW  g_IndexBufferView;


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

// We only namespace after variables to make it obvious where 
// ComPtr truly originated from. Plus general explicitness on 
// variable creation. 
using namespace Microsoft::WRL;

void Graphics::Init(int32_t width, int32_t height)
{
    ENGINE_CORE_DEBUG("Current Graphics API: Direct3D12\n");
    
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
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 1;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        (g_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)));
    }


    // Create command allocator.
    {
        ThrowIfFailed(g_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_CommandAllocator)));
    }

    CreateRenderTarget();
}

void LoadAssets()
{
    // Create empty root signature 
    {
        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        Microsoft::WRL::ComPtr<ID3DBlob> signature;
        Microsoft::WRL::ComPtr<ID3DBlob> error;
        ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
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
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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
    }

    // Create command list. (version "1" automatically closes itself - one less step). 
    g_Device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&g_CommandList));
    
    // Create the vertex buffer.
    {
        // Define the geometry for a triangle.
        Vertex triangleVertices[] =
        {
            // Clockwise.
            { {  0.0f,   0.25f * g_aspectRatio, 0.0f}, { 0.8f, 0.0f, 0.0f, 1.0f } }, // top
            { {  0.25f, -0.25f * g_aspectRatio, 0.0f}, { 0.0f, 0.8f, 0.0f, 1.0f } }, // bottom right
            { { -0.25f, -0.25f * g_aspectRatio, 0.0f}, { 0.0f, 0.0f, 0.8f, 1.0f } }, // bottom left

            // Second triangle 
            { {  0.60f,  0.25f * g_aspectRatio, 0.0f}, { 0.8f, 0.0f, 0.8f, 1.0f } }, // top
            { {  0.85f, -0.25f * g_aspectRatio, 0.0f}, { 0.0f, 0.8f, 0.8f, 1.0f } }, // bottom right
            { {  0.35f, -0.25f * g_aspectRatio, 0.0f}, { 0.8f, 0.8f, 0.0f, 1.0f } }, // bottom left
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
            0, 1, 2, // 1st triangle 
            3, 4, 5  // 2nd triangle  
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

void Graphics::Update()
{

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

void Graphics::Shutdown()
{
    CleanupRenderTarget();

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
    g_CommandList->RSSetViewports(1, &g_Viewport);
    g_CommandList->RSSetScissorRects(1, &g_ScissorRect);

    // Render Dear ImGui graphics
    const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
    g_CommandList->ClearRenderTargetView(g_mainRenderTargetDescriptor[backBufferIdx], clear_color_with_alpha, 0, NULL);
    g_CommandList->OMSetRenderTargets(1, &g_mainRenderTargetDescriptor[backBufferIdx], FALSE, NULL);
    
    g_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_CommandList->IASetVertexBuffers(0, 1, &g_VertexBufferView);
    g_CommandList->IASetIndexBuffer(&g_IndexBufferView);
    g_CommandList->DrawIndexedInstanced(6, 1, 0, 0, 0); 
    
    g_CommandList->SetDescriptorHeaps(1, g_pd3dSrvDescHeap.GetAddressOf());
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_CommandList.Get());
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
#endif
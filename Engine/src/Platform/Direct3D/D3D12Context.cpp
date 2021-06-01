//*********************************************************
// Copyright (c) 2021 Rachel Veer.
// Licensed under the Apache-2.0 License.
//*********************************************************
#include "pch.h"
#include "Engine/Core.h"
#include "Engine/GraphicsContext.h"
#include "Engine/ImGui/imgui_impl_dx12.h"
#include "Platform/Platform.h"

// DirectX specific code & libraries will only link/compile
// relative to the graphics layer if it's actually defined.

#if defined(DirectX) || defined(D3D) || defined(D3D12)

// Linking necessary libraries.
#pragma comment (lib, "D3d12.lib")
#pragma comment (lib, "dxgi.lib")

// Com & D3D headers.
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>

// D3D12 Helper functions.
#include "Platform/Direct3D/Utils/d3dx12.h" 
#include "Platform/Direct3D/Utils/DXHelper.h"

static const uint32_t m_FrameCount = 2;

ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);


// Pipeline objects.
Microsoft::WRL::ComPtr<IDXGISwapChain3> m_SwapChain;
Microsoft::WRL::ComPtr<ID3D12Device4> m_Device;
Microsoft::WRL::ComPtr<IDXGIFactory4> m_Factory;
Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue;
Microsoft::WRL::ComPtr<ID3D12Resource> m_RenderTargets[m_FrameCount];
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pd3dSrvDescHeap;
Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CommandList;
uint32_t m_rtvDescriptorSize;

static D3D12_CPU_DESCRIPTOR_HANDLE  g_mainRenderTargetDescriptor[m_FrameCount] = {};

// Synchronization objects.
uint32_t m_FrameIndex;
HANDLE m_FenceEvent;
Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
uint64_t m_FenceValue = 0;

HWND m_StoredHwnd;

void CreateRenderTarget();
void CleanupRenderTarget();
void LoadPipeline();
void LoadAssets();
void GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter,
    bool requestHighPerformanceAdapter);
void WaitForPreviousFrame();
void PopulateCommandList();

void Graphics::Init()
{
    ENGINE_CORE_DEBUG("Current Graphics API: Direct3D12\n");
    
    m_StoredHwnd = GetActiveWindow();
    LoadPipeline();
    LoadAssets();
    ImGui_ImplDX12_Init(m_Device.Get(), m_FrameCount,
        DXGI_FORMAT_R8G8B8A8_UNORM, m_pd3dSrvDescHeap.Get(),
        m_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
        m_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());
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
        ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&m_Factory)));

        Microsoft::WRL::ComPtr<IDXGIAdapter1> hardwareAdapter;
        GetHardwareAdapter(m_Factory.Get(), &hardwareAdapter, true);

        ThrowIfFailed(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_Device)));
    }

    // Create the command queue. 
    {
        D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
        cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

        ThrowIfFailed(m_Device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&m_CommandQueue)));
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
    swapChainDesc.BufferCount = m_FrameCount;
    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE; // Don't worry about transparency (for now).
    swapChainDesc.Flags = 0;

    Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(m_Factory->CreateSwapChainForHwnd(
        m_CommandQueue.Get(), // In D3D12, it points to a direct Command Queue.
        m_StoredHwnd,
        &swapChainDesc,
        nullptr,              // Not worrying about fullscreen capabilities as of now. 
        nullptr,
        &swapChain
    ));

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(m_Factory->MakeWindowAssociation(m_StoredHwnd, DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain.As(&m_SwapChain));
    m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();

    // Create a render target view (RTV) descriptor heap.
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = m_FrameCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(m_Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

        m_rtvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    // Create frame resources (RTV for each frame).
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

        // Create a RTV for each frame.
        for (uint32_t n = 0; n < m_FrameCount; n++)
        {
            g_mainRenderTargetDescriptor[n] = rtvHandle;
            rtvHandle.ptr += m_rtvDescriptorSize;
        }
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 1;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        (m_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_pd3dSrvDescHeap)));
    }


    // Create command allocator.
    {
        ThrowIfFailed(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocator)));
    }

    CreateRenderTarget();
}

void CreateRenderTarget()
{
    for (UINT i = 0; i < m_FrameCount; i++)
    {
        ID3D12Resource* pBackBuffer = NULL;
        m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
        m_Device->CreateRenderTargetView(pBackBuffer, NULL, g_mainRenderTargetDescriptor[i]);
        m_RenderTargets[i] = pBackBuffer;
    }
}

void CleanupRenderTarget()
{
    WaitForPreviousFrame();
    for (UINT i = 0; i < m_FrameCount; i++)
        if (m_RenderTargets[i]) { m_RenderTargets[i]->Release(); m_RenderTargets[i] = nullptr; }
}

void LoadAssets()
{
    // Create command list.
    m_Device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&m_CommandList));

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        ThrowIfFailed(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));
        m_FenceValue = 1;

        // Create an event handle to use for frame synchronization.
        m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_FenceEvent == nullptr)
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

void Graphics::Render()
{
    // Record all the commands we need to render the scene into the command list.
    PopulateCommandList();

    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { m_CommandList.Get() };
    m_CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Present the frame.
    ThrowIfFailed(m_SwapChain->Present(1, 0));

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
    ThrowIfFailed(m_CommandAllocator->Reset());

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_FrameIndex, m_rtvDescriptorSize);
  
    UINT backBufferIdx = m_SwapChain->GetCurrentBackBufferIndex();

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = m_RenderTargets[backBufferIdx].Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    m_CommandList->Reset(m_CommandAllocator.Get(), NULL);
    m_CommandList->ResourceBarrier(1, &barrier);

    // Render Dear ImGui graphics
    const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
    m_CommandList->ClearRenderTargetView(g_mainRenderTargetDescriptor[backBufferIdx], clear_color_with_alpha, 0, NULL);
    m_CommandList->OMSetRenderTargets(1, &g_mainRenderTargetDescriptor[backBufferIdx], FALSE, NULL);
    m_CommandList->SetDescriptorHeaps(1, m_pd3dSrvDescHeap.GetAddressOf());
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_CommandList.Get());
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    m_CommandList->ResourceBarrier(1, &barrier);

    ThrowIfFailed(m_CommandList->Close());
}

void WaitForPreviousFrame()
{
    // Warning: Waiting for frame completion is not best practice.
    // This is just for simplicity, I will implement a more elegant
    // example from the D3D12 samples repository. 

    // Signal and increment the fence value.
    const uint64_t fence = m_FenceValue;
    ThrowIfFailed(m_CommandQueue->Signal(m_Fence.Get(), fence));
    m_FenceValue++;

    // Wait until the previous frame is finished.
    if (m_Fence->GetCompletedValue() < fence)
    {
        ThrowIfFailed(m_Fence->SetEventOnCompletion(fence, m_FenceEvent));
        WaitForSingleObject(m_FenceEvent, INFINITE);
    }

    m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();
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
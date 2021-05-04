//*********************************************************
// Copyright (c) 2021 Rachel Veer.
// Licensed under the Apache-2.0 License.
//*********************************************************

#include "pch.h"
#include <wrl.h>    // For COM pointers. 
#include <d3d12.h>
#include "d3dx12.h" // D3D12 Helper functions. 
#include <dxgi1_6.h>

#include "Window.h"
#include "DXHelper.h"

#pragma comment (lib, "D3d12.lib")
#pragma comment (lib, "dxgi.lib")
// If linker issues, may have to link against very obscure guid.lib

// Forward declared functions.
void GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter,
    bool requestHighPerformanceAdapter);
void WaitForPreviousFrame();
void PopulateCommandList();
void OnUpdate();
void OnRender();

// Globals
static const UINT FrameCount = 2;

// Pipeline objects.
Microsoft::WRL::ComPtr<IDXGISwapChain3> g_SwapChain;
Microsoft::WRL::ComPtr<ID3D12Device4> g_Device;
Microsoft::WRL::ComPtr<IDXGIFactory4> g_Factory;
Microsoft::WRL::ComPtr<ID3D12CommandQueue> g_CommandQueue;
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_rtvHeap;
Microsoft::WRL::ComPtr<ID3D12CommandAllocator> g_CommandAllocator;
Microsoft::WRL::ComPtr<ID3D12RootSignature> g_RootSignature;
Microsoft::WRL::ComPtr<ID3D12PipelineState> g_PipelineState;
Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> g_CommandList;
uint32_t rtvDescriptorSize;

// Synchronization objects.
uint32_t g_FrameIndex;
HANDLE g_FenceEvent;
Microsoft::WRL::ComPtr<ID3D12Fence> g_Fence;
uint64_t FenceValue = 0;

Microsoft::WRL::ComPtr<ID3D12Resource> g_RenderTargets[FrameCount];

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ PWSTR pCmdLine,
    _In_ int nCmdShow)
{
    // Init window.
    Window wnd;

    // =========================
    //          D3D12
    // =========================

    // Init Pipeline //
    // ============= //
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
        swapChainDesc.BufferCount = FrameCount;
        swapChainDesc.Scaling = DXGI_SCALING_NONE;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE; // Don't worry about transparency (for now).
        swapChainDesc.Flags = 0;

        Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
        ThrowIfFailed(g_Factory->CreateSwapChainForHwnd(
            g_CommandQueue.Get(), // In D3D12, it points to a direct Command Queue.
            wnd.GetHwnd(),
            &swapChainDesc,
            nullptr,              // Not worrying about fullscreen capabilities as of now. 
            nullptr,
            &swapChain
        ));

        // This sample does not support fullscreen transitions.
        ThrowIfFailed(g_Factory->MakeWindowAssociation(wnd.GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

        ThrowIfFailed(swapChain.As(&g_SwapChain));
        g_FrameIndex = g_SwapChain->GetCurrentBackBufferIndex();

        // Create a render target view (RTV) descriptor heap.
        {
            D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
            rtvHeapDesc.NumDescriptors = FrameCount;
            rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            ThrowIfFailed(g_Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&g_rtvHeap)));

            rtvDescriptorSize = g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        }

        // Create frame resources (RTV for each frame).
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(g_rtvHeap->GetCPUDescriptorHandleForHeapStart());

            // Create a RTV for each frame.
            for (uint32_t n = 0; n < FrameCount; n++)
            {
                ThrowIfFailed(g_SwapChain->GetBuffer(n, IID_PPV_ARGS(&g_RenderTargets[n])));
                g_Device->CreateRenderTargetView(g_RenderTargets[n].Get(), nullptr, rtvHandle);
                rtvHandle.Offset(1, rtvDescriptorSize);
            }
        }

        // Create command allocator.
        {
            ThrowIfFailed(g_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_CommandAllocator)));
        }
    }

    // Init Assets //
    // =========== //
    {
        // Create command list.
        g_Device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&g_CommandList));

        // Create synchronization objects and wait until assets have been uploaded to the GPU.
        {
            ThrowIfFailed(g_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_Fence)));
            FenceValue = 1;

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
    // =========================

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
            OnUpdate();
            OnRender();
        }
    }

    return 0;
}

void OnUpdate()
{

}

void OnRender()
{
    // Record all the commands we need to render the scene into the command list.
    PopulateCommandList();

    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { g_CommandList.Get() };
    g_CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Present the frame.
    ThrowIfFailed(g_SwapChain->Present(1, 0));

    WaitForPreviousFrame();
}

void PopulateCommandList()
{
    // Command list allocators can only be reset when the associated
    // command lists have finished execution on the GPU; apps should use
    // fences to determine GPU execution progress.
    ThrowIfFailed(g_CommandAllocator->Reset());

    // However, when ExecuteCommandList() is called on a particular command
    // list, that command list can then be reset at any time and must be before
    // re-recording. 
    ThrowIfFailed(g_CommandList->Reset(g_CommandAllocator.Get(), g_PipelineState.Get()));

    // Indicate that the back buffer will be used as a render target.
    g_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(g_RenderTargets[g_FrameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(g_rtvHeap->GetCPUDescriptorHandleForHeapStart(), g_FrameIndex, rtvDescriptorSize);

    // Record commands.
    const float clearColor[] = { 1.0f, 0.3f, 0.4f, 1.0f };
    g_CommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    ThrowIfFailed(g_CommandList->Close());
}

void WaitForPreviousFrame()
{
    // Warning: Waiting for frame completion is not best practice.
    // This is just for simplicity, I will implement a more elegant
    // example from the D3D12 samples repository. 

    // Signal and increment the fence value.
    const uint64_t fence = FenceValue;
    ThrowIfFailed(g_CommandQueue->Signal(g_Fence.Get(), fence));
    FenceValue++;

    // Wait until the previous frame is finished.
    if (g_Fence->GetCompletedValue() < fence)
    {
        ThrowIfFailed(g_Fence->SetEventOnCompletion(fence, g_FenceEvent));
        WaitForSingleObject(g_FenceEvent, INFINITE);
    }

    g_FrameIndex = g_SwapChain->GetCurrentBackBufferIndex();
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

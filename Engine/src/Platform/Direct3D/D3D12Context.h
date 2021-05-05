//*********************************************************
// Copyright (c) 2021 Rachel Veer.
// Licensed under the Apache-2.0 License.
//*********************************************************

#pragma once

// Linking necessary libraries.
#pragma comment (lib, "D3d12.lib")
#pragma comment (lib, "dxgi.lib")

// Com & D3D headers.
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>

// D3D12 Helper functions.
#include "Engine/Utils/d3dx12.h" 
#include "Engine/Utils/DXHelper.h"

#include "Platform/Windows/Window.h"

class Direct3D
{
public:
    Direct3D(Window& wnd);
    ~Direct3D();
    void LoadPipeline();
    void LoadAssets();
    void OnUpdate();
    void OnRender();

private:
    static const uint32_t FrameCount = 2;

    // Pipeline objects.
    Microsoft::WRL::ComPtr<IDXGISwapChain3> g_SwapChain;
    Microsoft::WRL::ComPtr<ID3D12Device4> g_Device;
    Microsoft::WRL::ComPtr<IDXGIFactory4> g_Factory;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> g_CommandQueue;
    Microsoft::WRL::ComPtr<ID3D12Resource> g_RenderTargets[FrameCount];
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

    HWND storedHwnd;

    void GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter,
        bool requestHighPerformanceAdapter);
    void WaitForPreviousFrame();
    void PopulateCommandList();
};

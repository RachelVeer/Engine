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
#include "Platform/Direct3D/Utils/d3dx12.h" 
#include "Platform/Direct3D/Utils/DXHelper.h"

//#include "Platform/Windows/Window.h"
#include "Platform/Platform.h"

class Direct3D
{
public:
    Direct3D();
    ~Direct3D();
    void Init();
    void LoadPipeline();
    void LoadAssets();
    void OnUpdate();
    void OnRender();

private:
    static const uint32_t m_FrameCount = 2;

    // Pipeline objects.
    Microsoft::WRL::ComPtr<IDXGISwapChain3> m_SwapChain;
    Microsoft::WRL::ComPtr<ID3D12Device4> m_Device;
    Microsoft::WRL::ComPtr<IDXGIFactory4> m_Factory;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_RenderTargets[m_FrameCount];
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CommandList;
    uint32_t m_rtvDescriptorSize;

    // Synchronization objects.
    uint32_t m_FrameIndex;
    HANDLE m_FenceEvent;
    Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
    uint64_t m_FenceValue = 0;

    HWND m_StoredHwnd;

    void GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter,
        bool requestHighPerformanceAdapter);
    void WaitForPreviousFrame();
    void PopulateCommandList();
};

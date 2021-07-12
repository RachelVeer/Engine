//=========================
// Our Direct3D 12 Context.                    
//=========================
module;
// Platform specific.
#include <wincodec.h>
// D3D12.
#include "D3D12Bridge.h"
// Logger.
#include "spdlog/sinks/stdout_color_sinks.h"

using namespace Microsoft::WRL;
using namespace DirectX;
module D3D12Context;

void D3D12Context::Init(int32_t width, int32_t height)
{
    D3D12ContextMod();
    CoreLoggerDebug("Current Graphics API: Direct3D12.");
    HelloDirectXMath();

    // Storing incoming/external data.
    g_StoredHwnd = static_cast<HWND>(Platform::getAdditionalPlatformData());
    surface.width = width;
    surface.height = height;

    // Now preparing data for pipeline.
    g_aspectRatio = static_cast<float>(surface.width) / static_cast<float>(surface.height);

    g_Viewport.Width = (float)surface.width;
    g_Viewport.Height = (float)surface.height;
    g_Viewport.MinDepth = 0.0f;
    g_Viewport.MaxDepth = 1.0f;
    g_Viewport.TopLeftX = 0.0f;
    g_Viewport.TopLeftY = 0.0f;
    g_ScissorRect.right = surface.width;
    g_ScissorRect.bottom = surface.height;

    // Then we can call upon the actual api. 
    LoadPipeline();
    LoadAssets();
    // Dear Imgui is our third descriptor within our main Shader Resource View Descriptor Heap. (Starting from 0 -> 1 -> 2 (2 being our "third" one)).
    CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandleCPU(g_srvHeap->GetCPUDescriptorHandleForHeapStart(), 3, g_srvDescriptorSize);
    CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandleGPU(g_srvHeap->GetGPUDescriptorHandleForHeapStart(), 3, g_srvDescriptorSize);
    ImGui_ImplDX12_Init(g_Device.Get(), g_FrameCount,
        DXGI_FORMAT_R8G8B8A8_UNORM, g_srvHeap.Get(),
        srvHandleCPU, srvHandleGPU);
}


// Update frame-based values.
void D3D12Context::Update(float color[], bool adjustOffset, float angle)
{    
    clear_color = { color[0], color[1], color[2], color[3] };
    
    // Do we want to move our geometry in the first place?
    if (adjustOffset)
    {
        // By default it moves forward, thus once we reach offsetBounds - set it false.
        /*if (g_constantBufferData.offset.x > cbvParams.offsetBounds) { cbvParams.forward = false; }
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

        //g_constantBufferData.cbcolor.y = color.g;
        */
        
        if (Platform::getUpArrowKey())
        {
            g_LerpCBData.mixColor += cbvParams.translationSpeed;
        }
        
        if (Platform::getDownArrowKey())
        {
            g_LerpCBData.mixColor -= cbvParams.translationSpeed;
        }
        
        // Explicit initialization of identity matrix. 
        XMMATRIX trans = DirectX::XMMatrixIdentity();

        // Creating transformation matrix.
        trans = DirectX::XMMatrixTranspose(
            // Scale -> Rotation -> Translation.
            XMMatrixScaling(1.0f, 1.0f, 1.0f) *
            XMMatrixRotationZ(angle) *
            XMMatrixTranslation(0.5f, -0.5f, 0.0f)
            );

        g_constantBufferData.transform = trans;

        memcpy(g_pCbvDataBegin, &g_constantBufferData, sizeof(g_constantBufferData));

        // Explicit initialization of identity matrix. 
        trans = DirectX::XMMatrixIdentity();

        // Creating transformation matrix.
        trans = DirectX::XMMatrixTranspose(
            // Scale -> Rotation -> Translation.
            XMMatrixScaling(1.0f, 1.0f, 1.0f) *
            XMMatrixRotationZ(-angle * 0.5f) *
            XMMatrixTranslation(-0.5f, 0.5f, 0.0f)
        );

        g_constantBufferData.transform = trans;

        memcpy(g_pCbvDataBegin + sizeof(SceneConstantBuffer), &g_constantBufferData, sizeof(g_constantBufferData));

        memcpy(g_pLerpCbvDataBegin, &g_LerpCBData, sizeof(g_LerpCBData));
    }
}

void D3D12Context::Render()
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

void D3D12Context::Screenshot()
{
    UINT backBufferIdx = g_SwapChain->GetCurrentBackBufferIndex();
    SaveWICTextureToFile(g_CommandQueue.Get(), g_RenderTargets[backBufferIdx].Get(), GUID_ContainerFormatJpeg, L"Screenshot.jpeg", D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_PRESENT);
}

void D3D12Context::Shutdown()
{
    CleanupRenderTarget();
}

void LoadPipeline()
{
    EnableDebugLayer();
    CreateDevice();
    CreateCommandQueue();
    CreateSwapChain();
    CreateDescriptorHeaps();
    CreateRenderTarget();
    CreateCommandAllocator();
}

void LoadAssets()
{
    CreateRootSignatureAndHeapContents();
    CreatePipelineState();
    CreateCommandList();
    CreateVertexBuffer();
    CreateIndexBuffer();
    CreateConstantBuffers(); // Take heed of sudden plurals,
    CreateTextures();        // we're handling multiple x's now.
    //^----Handles: "CreateSyncObjectsAndWaitForAssetUpload()".
    BuildMatrices();
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

    ID3D12DescriptorHeap* ppHeaps[] = { g_srvHeap.Get(), g_samplerHeap.Get() };
    g_CommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    // Set Texture 1
    CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(g_srvHeap->GetGPUDescriptorHandleForHeapStart(), 1, g_srvDescriptorSize);
    g_CommandList->SetGraphicsRootDescriptorTable(1, srvHandle);
    // Get a proper handle to our second descriptor.
    CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle2(g_srvHeap->GetGPUDescriptorHandleForHeapStart(), 2, g_srvDescriptorSize);
    // Then set Texture 2
    g_CommandList->SetGraphicsRootDescriptorTable(2, srvHandle2);

    CD3DX12_GPU_DESCRIPTOR_HANDLE sampleHandle(g_samplerHeap->GetGPUDescriptorHandleForHeapStart(), 1, g_samplerDescriptorSize);
    g_CommandList->SetGraphicsRootDescriptorTable(3, g_samplerHeap->GetGPUDescriptorHandleForHeapStart());
    //g_CommandList->SetGraphicsRootDescriptorTable(3,sampleHandle);
    g_CommandList->SetGraphicsRootDescriptorTable(4, sampleHandle);

    g_CommandList->SetGraphicsRoot32BitConstants(5, 2, g_pLerpCbvDataBegin, 0);

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

    // Constant Buffer View
    g_CommandList->SetGraphicsRootConstantBufferView(0, g_ConstantBuffer->GetGPUVirtualAddress());
    g_CommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);


    // Second Triangle
    // Constant Buffer View
    g_CommandList->SetGraphicsRootConstantBufferView(0, g_ConstantBuffer->GetGPUVirtualAddress() + sizeof(SceneConstantBuffer));
    g_CommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

    // Related to Imgui.
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
void GetHardwareAdapter(IDXGIFactory4* pFactory, IDXGIAdapter1** ppAdapter,
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

//===========//
// Pipeline. //
//===========//

void EnableDebugLayer()
{
    // This must be called before the D3D12 device is created. 
    // Otherwise the D3D12 device will be removed. 
    Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
    }
}

void CreateDevice()
{
    // Don't mix the use of DXGI 1.0 (IDXGIFactory) and DXGI 1.1 (IDXGIFactory1) in an application.
    ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&g_Factory)));

    Microsoft::WRL::ComPtr<IDXGIAdapter1> hardwareAdapter;
    GetHardwareAdapter(g_Factory.Get(), &hardwareAdapter, true);

    ThrowIfFailed(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&g_Device)));
}

void CreateCommandQueue()
{
    D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
    cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    ThrowIfFailed(g_Device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&g_CommandQueue)));
}

void CreateSwapChain()
{
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
}

void CreateDescriptorHeaps()
{
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
        // Describe and create a shader resource view (SRV) heap for the texture.
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
        srvHeapDesc.NumDescriptors = 4; // CBV is 1, Texture 1 is our 2nd descriptor, Texture 2 our 3rd, and lastly Dear Imgui (Though I'd like it to be first). 
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        ThrowIfFailed(g_Device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&g_srvHeap)));
        g_srvDescriptorSize = g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    {
        // Describe and create a shader resource view (SRV) heap for the texture.
        D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
        samplerHeapDesc.NumDescriptors = 2;
        samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        ThrowIfFailed(g_Device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&g_samplerHeap)));
        g_samplerDescriptorSize = g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    }
}

void CreateCommandAllocator()
{
    ThrowIfFailed(g_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_CommandAllocator)));
}

//=========//
// Assets. //
//=========//
void CreateRootSignatureAndHeapContents()
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
    const int Tex1Range = 0;
    const int Tex2Range = 1;
    const int Sampler1Range = 2;
    const int Sampler2Range = 3;
    const int maxRanges = 4;

    const int CBVParam  = 0;
    const int Tex1Param = 1;
    const int Tex2Param = 2;
    const int Sampler1Param = 3;
    const int Sampler2Param = 4;
    const int LerpConstantParam = 5;
    const int maxParams = 6;
    
    CD3DX12_DESCRIPTOR_RANGE1 ranges[maxRanges];
    CD3DX12_ROOT_PARAMETER1 rootParameters[maxParams];

    // Constant Buffer View, as a Root Descriptor (...!= Descriptor table).
    rootParameters[CBVParam].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);

    // Texture 1
    ranges[Tex1Range].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
    // Descriptor table, which will point to our SRV descriptor within our original SRV descriptor heap. 
    rootParameters[Tex1Param].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

    // Texture 2
    // Notice its baseShaderRegister is set 1, as next texture register is (t1)
    ranges[Tex2Range].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
    // Descriptor table, which will point to our SRV descriptor within our original SRV descriptor heap. 
    rootParameters[Tex2Param].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);

    // Samplers
    ranges[Sampler1Range].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
    rootParameters[Sampler1Param].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);

    ranges[Sampler2Range].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 1);
    rootParameters[Sampler2Param].InitAsDescriptorTable(1, &ranges[3], D3D12_SHADER_VISIBILITY_PIXEL);

    // Our Lerp constant. 
    rootParameters[LerpConstantParam].InitAsConstants(2, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);

    D3D12_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.MipLODBias = 0;
    sampler.MaxAnisotropy = 0;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor[0] = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler.BorderColor[1] = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler.BorderColor[2] = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler.BorderColor[3] = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    g_Device->CreateSampler(&sampler, g_samplerHeap->GetCPUDescriptorHandleForHeapStart());

    D3D12_SAMPLER_DESC sampler2 = {};
    sampler2.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    sampler2.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler2.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler2.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler2.MipLODBias = 0;
    sampler2.MaxAnisotropy = 0;
    sampler2.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler2.MinLOD = 0.0f;
    sampler2.MaxLOD = D3D12_FLOAT32_MAX;
    sampler2.BorderColor[0] = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler2.BorderColor[1] = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler2.BorderColor[2] = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler2.BorderColor[3] = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;

    CD3DX12_CPU_DESCRIPTOR_HANDLE samplerHandle(g_samplerHeap->GetCPUDescriptorHandleForHeapStart(), 1, g_samplerDescriptorSize);
    g_Device->CreateSampler(&sampler2, samplerHandle);

    // Allow input layout and deny uneccessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
    ThrowIfFailed(g_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&g_RootSignature)));
}

void CreatePipelineState()
{
    // Create the pipeline state, which includes compiling and loading shaders.
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

}

void CreateCommandList()
{
    // Create command list. (version "1" automatically closes itself - one less step). 
    g_Device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&g_CommandList));
}

void CreateVertexBuffer()
{
    // Define the geometry for a triangle.
    Vertex triangleVertices[] =
    {
        // Clockwise.
        { { -0.25f,  0.25f, 0.0f}, { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } }, // top left
        { {  0.25f, -0.25f, 0.0f}, { 0.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } }, // bottom right
        { { -0.25f, -0.25f, 0.0f}, { 0.0f, 0.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } }, // bottom left
        { {  0.25f,  0.25f, 0.0f}, { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } }, // top right
    };

    const uint32_t vertexBufferSize = sizeof(triangleVertices);

    // Note: using upload heaps to transfer static data like vert buffers is not 
    // recommended. Every time the GPU needs it, the upload heap will be marshalled 
    // over. Please read up on Default Heap usage. An upload heap is used here for 
    // code simplicity and because there are very few verts to actually transfer.
    // autos = & pattern to avoid visual studio C2102 error.
    auto uploadProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
    ThrowIfFailed(g_Device->CreateCommittedResource(
        &uploadProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&g_VertexBuffer)));

    // Copy the triangle data to the vertex buffer.
    UINT8* pVertexDataBegin = { 0 };
    // We do not intend to read from this resource on the CPU.
    CD3DX12_RANGE readRange(0, 0);
    ThrowIfFailed(g_VertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
    memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
    g_VertexBuffer->Unmap(0, nullptr);

    // Initialize the vertex buffer view.
    g_VertexBufferView.BufferLocation = g_VertexBuffer->GetGPUVirtualAddress();
    g_VertexBufferView.StrideInBytes = sizeof(Vertex);
    g_VertexBufferView.SizeInBytes = vertexBufferSize;

}

void CreateIndexBuffer()
{
    // Define indices 
    int16_t Indices[] =
    {
        0, 1, 2,
        0, 3, 1
    };

    const uint32_t indexBufferSize = sizeof(Indices);

    auto uploadProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

    ThrowIfFailed(g_Device->CreateCommittedResource(
        &uploadProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
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

void CreateConstantBuffers()
{
    // Create the constant buffer.
    {
        const UINT constantBufferSize = sizeof(SceneConstantBuffer); // CB size is required to be 256-byte aligned.
        const uint64_t AlignedKB = 1024 * 64; // Must be 64KB aligned. 
        auto uploadProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(AlignedKB);
        ThrowIfFailed(g_Device->CreateCommittedResource(
            &uploadProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&g_ConstantBuffer)));

        // Map and intialize the constant buffer. We don't unmap this until the
        // app closes. Keeping things mapped for the lifetime of the resource is okay.
        CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource of the CPU.
        ThrowIfFailed(g_ConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&g_pCbvDataBegin)));
        memcpy(g_pCbvDataBegin, &g_constantBufferData, sizeof(g_constantBufferData));
        memcpy(g_pCbvDataBegin + sizeof(SceneConstantBuffer), &g_constantBufferData, sizeof(g_constantBufferData));

        // Feed data immediately to not make square appearance dependent on enabling update loop.
        // Explicit initialization of identity matrix. 
        //XMMATRIX trans = DirectX::XMMatrixIdentity();

        //g_constantBufferData.transform = trans;

        //memcpy(g_pCbvDataBegin, &g_constantBufferData, sizeof(g_constantBufferData));
    }

    {
        //g_LerpCBV
        const UINT constantBufferSize = sizeof(Lerp); // CB size is required to be 256-byte aligned.
        auto uploadProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);
        ThrowIfFailed(g_Device->CreateCommittedResource(
            &uploadProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&g_LerpConstantBuffer)));

        // NOTE: No constant buffer view is being created here anymore - 
        // as the rootsignature handles it being 'set' as a rootconstant now.

        // Map and intialize the constant buffer. We don't unmap this until the
        // app closes. Keeping things mapped for the lifetime of the resource is okay.
        CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource of the CPU.
        ThrowIfFailed(g_LerpConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&g_pLerpCbvDataBegin)));
        memcpy(g_pLerpCbvDataBegin, &g_LerpCBData, sizeof(g_LerpCBData));
    }
}

void CreateTextures()
{
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

        {
            auto uploadProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
            ThrowIfFailed(g_Device->CreateCommittedResource(
                &uploadProperties,
                D3D12_HEAP_FLAG_NONE,
                &textureDesc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(&g_Texture)
            ));
        }
        // Copy data to the intermediate upload heap and then schedule a copy
        // from the upload heap to the Texture2D.
        std::unique_ptr<uint8_t[]> decodedData;
        D3D12_SUBRESOURCE_DATA subresouce;
        //LoadWICTextureFromFile(g_Device.Get(), L"container.jpg", &g_Texture, decodedData, subresouce);
        LoadWICTextureFromFile(g_Device.Get(), L"container.jpg", &g_Texture, decodedData, subresouce);


        const UINT64 uploadBufferSize = GetRequiredIntermediateSize(g_Texture.Get(), 0, 1);

        {
            auto uploadProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
            // Create the GPU upload buffer.
            ThrowIfFailed(g_Device->CreateCommittedResource(
                &uploadProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&textureUploadHeap)
            ));
        }

        // CommandList1 closes itself after creation, so we reset commandlist to call UpdateSubresources.
        g_CommandList->Reset(g_CommandAllocator.Get(), g_PipelineState.Get());
        // Copy upload contents to default heap.
        UpdateSubresources(g_CommandList.Get(), g_Texture.Get(), textureUploadHeap.Get(), 0, 0, 1, &subresouce);
        // Transistion the texture default heap to pixel shader resource. 
        auto transition = CD3DX12_RESOURCE_BARRIER::Transition(g_Texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        g_CommandList->ResourceBarrier(1, &transition);
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

        CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(g_srvHeap->GetCPUDescriptorHandleForHeapStart(), 1, g_srvDescriptorSize);
        g_Device->CreateShaderResourceView(g_Texture.Get(), &srvDesc, srvHandle);
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

        auto uploadProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        ThrowIfFailed(g_Device->CreateCommittedResource(
            &uploadProperties,
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

        {
            auto uploadProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
            // Create the GPU upload buffer.
            ThrowIfFailed(g_Device->CreateCommittedResource(
                &uploadProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&textureUploadHeap2)
            ));
        }

        // CommandList1 closes itself after creation, so we reset commandlist to call UpdateSubresources.
        g_CommandList->Reset(g_CommandAllocator.Get(), g_PipelineState.Get());
        // Copy upload contents to default heap.
        UpdateSubresources(g_CommandList.Get(), g_Texture2.Get(), textureUploadHeap2.Get(), 0, 0, 1, &subresouce);
        // Transistion the texture default heap to pixel shader resource.
        auto transition = CD3DX12_RESOURCE_BARRIER::Transition(g_Texture2.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        g_CommandList->ResourceBarrier(1, &transition);
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
        // Here is how we access & store the texture into the SECOND descriptor of our Heap.
        // It's only "1" because of index counting ([0], [1]), either way - we include the proper
        // size of the descriptor heap type as well. 
        CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(g_srvHeap->GetCPUDescriptorHandleForHeapStart(), 2, g_srvDescriptorSize);
        g_Device->CreateShaderResourceView(g_Texture2.Get(), &srvDesc, srvHandle);
    }

    CreateSyncObjectsAndWaitForAssetUpload();
}

void CreateSyncObjectsAndWaitForAssetUpload()
{
    // Create synchronization objects and wait until assets have been uploaded to the GPU.
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

void BuildMatrices()
{
    // Explicit initialization of identity matrix. 
    XMMATRIX trans = DirectX::XMMatrixIdentity();

    // Creating transformation matrix.
    trans = DirectX::XMMatrixTranspose(
        // Scale -> Rotation -> Translation.
        XMMatrixScaling(1.0f, 1.0f, 1.0f) *
        XMMatrixTranslation(0.5f, -0.5f, 0.0f)
    );

    g_constantBufferData.transform = trans;

    memcpy(g_pCbvDataBegin, &g_constantBufferData, sizeof(g_constantBufferData));

    // Explicit initialization of identity matrix. 
    trans = DirectX::XMMatrixIdentity();

    // Creating transformation matrix.
    trans = DirectX::XMMatrixTranspose(
        // Scale -> Rotation -> Translation.
        XMMatrixScaling(1.0f, 1.0f, 1.0f) *
        XMMatrixTranslation(-0.5f, 0.5f, 0.0f)
    );

    g_constantBufferData.transform = trans;

    memcpy(g_pCbvDataBegin + sizeof(SceneConstantBuffer), &g_constantBufferData, sizeof(g_constantBufferData));
}
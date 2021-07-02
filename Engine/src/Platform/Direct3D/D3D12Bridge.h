#pragma once
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
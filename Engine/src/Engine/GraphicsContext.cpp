#include "pch.h"
#include "SandboxTypes.h"
#include "GraphicsContext.h"
#include "Platform/Direct3D/D3D12Context.h"
#include "Log.h"

Graphics* Graphics::CreateGraphics(SandboxState* sandboxInstance)
{
    switch (sandboxInstance->gfxAPI)
    {
        case GraphicsAPI::Unknown: 
        {
            ENGINE_CORE_ERROR("Unknown API!\n"); 
            return nullptr;
        }
        case GraphicsAPI::Direct3D12:
        {
            ENGINE_CORE_DEBUG("Selected API: Direct3D12\n");
            return new Direct3D();
        }
        default:
        {
            ENGINE_CORE_ERROR("Unknown API!\n");
            return nullptr;
        }
    }
}
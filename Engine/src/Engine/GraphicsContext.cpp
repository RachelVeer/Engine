#include "pch.h"
#include "SandboxTypes.h"
#include "GraphicsContext.h"
#include "Platform/Direct3D/D3D12Context.h"

Graphics* Graphics::CreateGraphics(SandboxState* sandboxInstance)
{
    switch (sandboxInstance->gfxAPI)
    {
        case GraphicsAPI::Unknown: 
        {
            printf("Unknown API!\n"); 
            return nullptr;
        }
        case GraphicsAPI::Direct3D12:
        {
            printf("Selected API: Direct3D12\n");
            return new Direct3D();
        }
    }
}
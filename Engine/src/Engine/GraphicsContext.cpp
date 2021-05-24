#include "pch.h"
#include "GraphicsContext.h"
#include "Platform/Direct3D/D3D12Context.h"

typedef struct GraphicsAPI {
    Direct3D D3D12;
} GraphicsAPI;

GraphicsAPI validAPI;

void Graphics::Init(GFXAPI& gfxAPI)
{
    switch (gfxAPI)
    {
        case GFXAPI::Unknown:
        {
            printf("Return type unknown!");
            break;
        }
        case GFXAPI::Direct3D12:
        {
            validAPI.D3D12.Init();
        }
    }
}


void Graphics::Update(GFXAPI& gfxAPI)
{
    switch (gfxAPI)
    {
        case GFXAPI::Unknown:
        {
            printf("Return type unknown!");
            break;
        }
        case GFXAPI::Direct3D12:
        {
            validAPI.D3D12.Update();
        }
    }
}

void Graphics::Render(GFXAPI& gfxAPI)
{
    switch (gfxAPI)
    {
        case GFXAPI::Unknown:
        {
            printf("Return type unknown!");
            break;
        }
        case GFXAPI::Direct3D12:
        {
            validAPI.D3D12.Render();
        }
    }
}
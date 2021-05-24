#include "pch.h"
#include "GraphicsContext.h"
#include "Platform/Direct3D/D3D12Context.h"

typedef struct GraphicsAPI
{
    Direct3D D3D12;
} GraphicsAPI;

static GraphicsAPI gfxAPI;

void Graphics::Init()
{
    gfxAPI.D3D12.Init();
}

void Graphics::Update()
{
    gfxAPI.D3D12.Update();
}

void Graphics::Render()
{
    gfxAPI.D3D12.Render();
}

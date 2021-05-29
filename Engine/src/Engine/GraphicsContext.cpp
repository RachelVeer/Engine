#include "pch.h"
#include "GraphicsContext.h"
#include "Platform/Direct3D/D3D12Context.h"

Graphics* Graphics::CreateGraphics()
{
    return new Direct3D();
}
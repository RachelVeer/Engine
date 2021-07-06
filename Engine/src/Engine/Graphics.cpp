module Graphics;

import <cstdint>;
import <cstdio>;

import D3D12Context; // Backend. 

void GraphicsTest()
{
    printf("Graphics Implementation module.\n");
}

void Graphics::Init(int32_t width, int32_t height)
{
    D3D12Context::Init(width, height);
}


// Update frame-based values.
void Graphics::Update(ClearColor& color, bool adjustOffset)
{
    D3D12Context::Update(color, adjustOffset);
}

void Graphics::Render()
{
    D3D12Context::Render();
}

void Graphics::Screenshot()
{
    D3D12Context::Screenshot();
}

void Graphics::Shutdown()
{
    D3D12Context::Shutdown();
}
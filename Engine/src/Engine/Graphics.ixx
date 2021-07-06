//*********************************************************
// Copyright (c) 2021 Rachel Veer.
// Licensed under the Apache-2.0 License.
//*********************************************************

//================//
//    Frontend    //
//================//
export module Graphics;

import <cstdint>;

export struct ClearColor
{
    float r, g, b, a;
};

// Test functions.
export void GraphicsTest();

// Current implementation: Platform/Direct3D/GraphicsD3D12.cpp!
namespace Graphics
{
    export void Init(int32_t width, int32_t height);
    export void Update(ClearColor& color, bool adjustOffset);
    export void Render();
    export void Shutdown();
    export void Screenshot();
};
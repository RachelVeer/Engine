//*********************************************************
// Copyright (c) 2021 Rachel Veer.
// Licensed under the Apache-2.0 License.
//*********************************************************

//================//
//    Frontend    //
//================//
module;
#include <cstdint>
export module Graphics;

// Test functions.
export void GraphicsTest();

// Current implementation: Platform/Direct3D/GraphicsD3D12.cpp!
namespace Graphics
{
    export void Init(int32_t width, int32_t height);
    export void Update(float color[], bool adjustOffset, float angle);
    export void Render();
    export void Shutdown();
    export void Screenshot();
};
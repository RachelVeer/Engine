//*********************************************************
// Copyright (c) 2021 Rachel Veer.
// Licensed under the Apache-2.0 License.
//*********************************************************
module;
#include <cstdint>
export module Graphics;

export struct ClearColor
{
    float r, g, b, a;
};

export void GraphicsTest();

namespace Graphics
{
    export void Init(int32_t width, int32_t height);
    export void Update(ClearColor& color, bool adjustOffset);
    export void Render(ClearColor& color);
    export void Shutdown();
    export void Screenshot();
};
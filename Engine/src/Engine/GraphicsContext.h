#pragma once

enum class GFXAPI
{
    Unknown = 0,
    Direct3D12 = 1,
};

class Graphics
{
public:
    void Init(GFXAPI& gfxAPI);
    void Update(GFXAPI& gfxAPI);
    void Render(GFXAPI& gfxAPI);
};
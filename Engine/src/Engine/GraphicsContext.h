#pragma once
//#include "SandboxTypes.h"

struct SandboxState;

enum class GraphicsAPI
{
    Unknown = 0,
    Direct3D12 = 1
};

class Graphics
{
public:
    void Init();
    void Update();
    void Render();
    ~Graphics() {}

    Graphics* CreateGraphics() { return new Graphics(); }
};
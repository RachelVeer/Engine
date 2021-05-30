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
    virtual void Init() = 0;
    virtual void Update() = 0;
    virtual void Render() = 0;
    virtual ~Graphics() {}

    Graphics* CreateGraphics(struct SandboxState* sandboxInstance);
};
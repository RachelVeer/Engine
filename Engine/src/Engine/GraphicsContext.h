#pragma once

class Graphics
{
public:
    virtual void Init() = 0;
    virtual void Update() = 0;
    virtual void Render() = 0;
    virtual ~Graphics() {}

    Graphics* CreateGraphics();
};
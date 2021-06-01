#pragma once

class Graphics
{
public:
    void Init();
    void Update();
    void Render();
    void Shutdown();
    ~Graphics() {}

    /* Graphics* CreateGraphics() { return new Graphics(); } */
};
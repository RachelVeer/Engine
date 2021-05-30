#pragma once

class Graphics
{
public:
    void Init();
    void Update();
    void Render();
    ~Graphics() {}

    /* Graphics* CreateGraphics() { return new Graphics(); } */
};
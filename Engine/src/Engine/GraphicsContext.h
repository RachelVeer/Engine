#pragma once

struct ClearColor
{
    float r, g, b, a;
};

class Graphics
{
public:
    Graphics() {}
    ~Graphics() {}
    void Init();
    void Update();
    void Render(ClearColor& color);
    void Shutdown();

    /* Graphics* CreateGraphics() { return new Graphics(); } */
};
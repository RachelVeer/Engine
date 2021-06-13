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
    void Init(int32_t width, int32_t height);
    void Update(ClearColor& color, bool adjustOffset);
    void Render(ClearColor& color);
    void Shutdown();
    void Screenshot();

    /* Graphics* CreateGraphics() { return new Graphics(); } */
};
#pragma once

namespace ui4d {

class Framebuffer {
public:
    Framebuffer() = default;
    Framebuffer(int width, int height);
    ~Framebuffer();

    void bind();
    void unbind();
    void resize(int width, int height);

    unsigned int getTexture() const { return textureId; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    unsigned int fboId = 0;
    unsigned int textureId = 0;
    unsigned int rboId = 0;
    int width = 0;
    int height = 0;
};

} // namespace ui4d
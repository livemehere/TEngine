#pragma once

#include <glad/glad.h>


class FrameBuffer {
    GLuint id = 0;
    GLuint textureId = 0;
    GLuint rboId = 0;
    int width;
    int height;
public:
    FrameBuffer(int width, int height);
    ~FrameBuffer();

    void resize(int newWidth, int newHeight);

    GLuint getTextureId() const { return textureId; }

    void bind();
    void unBind();
};

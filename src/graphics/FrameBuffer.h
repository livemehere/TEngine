#pragma once

#include <glad/glad.h>

#include "../rendering/RenderExtent.h"

class FrameBuffer {
    GLuint id = 0;
    GLuint textureId = 0;
    GLuint rboId = 0;
    int width = 0;
    int height = 0;

    void allocateAttachments();

public:
    explicit FrameBuffer(RenderExtent extent);
    ~FrameBuffer();

    FrameBuffer(const FrameBuffer&) = delete;
    FrameBuffer& operator=(const FrameBuffer&) = delete;

    void resize(RenderExtent extent);

    GLuint getTextureId() const { return textureId; }
    RenderExtent getExtent() const { return {width, height}; }

    void bind() const;
    static void bindDefault(RenderExtent windowExtent);
};

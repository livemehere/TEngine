#pragma once

#include <glad/glad.h>

#include "../rendering/RenderExtent.h"

struct FrameBufferSpecification {
    RenderExtent extent;
    bool hasDepthStencil = true;
};

class FrameBuffer {
    GLuint id = 0;
    GLuint textureId = 0;
    GLuint rboId = 0;
    int width = 0;
    int height = 0;
    bool hasDepthStencil = true;

    void allocateAttachments();

public:
    explicit FrameBuffer(FrameBufferSpecification specification);
    ~FrameBuffer();

    FrameBuffer(const FrameBuffer&) = delete;
    FrameBuffer& operator=(const FrameBuffer&) = delete;

    void resize(RenderExtent extent);

    GLuint getTextureId() const { return textureId; }
    RenderExtent getExtent() const { return {width, height}; }

    void bind() const;
    static void bindDefault(RenderExtent windowExtent);
};

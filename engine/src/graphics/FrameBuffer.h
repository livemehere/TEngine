#pragma once

#include <glad/glad.h>

#include "../rendering/RenderExtent.h"

struct FrameBufferSpecification {
    RenderExtent extent;
    bool hasDepthStencil = true;
    int samples = 1;
};

class FrameBuffer {
    GLuint id = 0;
    GLuint textureId = 0;
    GLuint rboId = 0;
    int width = 0;
    int height = 0;
    bool hasDepthStencil = true;
    int samples = 1;

    void allocateAttachments();
    void recreateColorTexture();

public:
    explicit FrameBuffer(FrameBufferSpecification specification);
    ~FrameBuffer();

    FrameBuffer(const FrameBuffer&) = delete;
    FrameBuffer& operator=(const FrameBuffer&) = delete;

    void resize(RenderExtent extent);
    void setSamples(int sampleCount);
    void resolveTo(FrameBuffer& destination) const;

    GLuint getTextureId() const { return textureId; }
    RenderExtent getExtent() const { return {width, height}; }
    int getSamples() const { return samples; }
    bool isMultisampled() const { return samples > 1; }

    void bind() const;
    static void bindDefault(RenderExtent windowExtent);
};

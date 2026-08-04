#pragma once

#include <glad/glad.h>

#include "../rendering/RenderExtent.h"

enum class FrameBufferColorFormat {
    RGBA8,
    RGBA16F
};

struct FrameBufferSpecification {
    RenderExtent extent;
    bool hasDepthStencil = true;
    int samples = 1;
    FrameBufferColorFormat colorFormat = FrameBufferColorFormat::RGBA8;
};

class FrameBuffer {
    GLuint id = 0;
    GLuint textureId = 0;
    GLuint rboId = 0;
    int width = 0;
    int height = 0;
    bool hasDepthStencil = true;
    int samples = 1;
    FrameBufferColorFormat colorFormat = FrameBufferColorFormat::RGBA8;

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
    FrameBufferColorFormat getColorFormat() const { return colorFormat; }
    bool isMultisampled() const { return samples > 1; }

    void bind() const;
    static void bindDefault(RenderExtent windowExtent);
};

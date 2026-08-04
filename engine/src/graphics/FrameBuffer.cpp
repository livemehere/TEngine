#include "FrameBuffer.h"

#include <algorithm>
#include <stdexcept>

FrameBuffer::FrameBuffer(const FrameBufferSpecification specification)
    : width(specification.extent.width),
      height(specification.extent.height),
      hasDepthStencil(specification.hasDepthStencil),
      samples(std::max(1, specification.samples)) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("Framebuffer extent must be positive");
    }

    GLint maximumSamples = 1;
    glGetIntegerv(GL_MAX_SAMPLES, &maximumSamples);
    samples = std::min(samples, maximumSamples);

    glGenFramebuffers(1, &id);
    glGenTextures(1, &textureId);
    if (hasDepthStencil) {
        glGenRenderbuffers(1, &rboId);
    }

    try {
        allocateAttachments();
    } catch (...) {
        glDeleteFramebuffers(1, &id);
        glDeleteTextures(1, &textureId);
        if (rboId != 0) {
            glDeleteRenderbuffers(1, &rboId);
        }
        throw;
    }
}

void FrameBuffer::recreateColorTexture() {
    if (textureId != 0) {
        glDeleteTextures(1, &textureId);
    }
    glGenTextures(1, &textureId);
}

FrameBuffer::~FrameBuffer() {
    glDeleteFramebuffers(1, &id);
    glDeleteTextures(1, &textureId);
    if (rboId != 0) {
        glDeleteRenderbuffers(1, &rboId);
    }
}

void FrameBuffer::allocateAttachments() {
    glBindFramebuffer(GL_FRAMEBUFFER, id);

    const GLenum textureTarget = isMultisampled()
                                     ? GL_TEXTURE_2D_MULTISAMPLE
                                     : GL_TEXTURE_2D;
    glBindTexture(textureTarget, textureId);
    if (isMultisampled()) {
        glTexImage2DMultisample(
            GL_TEXTURE_2D_MULTISAMPLE,
            samples,
            GL_RGBA8,
            width,
            height,
            GL_TRUE
        );
    } else {
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA8,
            width,
            height,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            nullptr
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        textureTarget,
        textureId,
        0
    );
    if (hasDepthStencil) {
        glBindRenderbuffer(GL_RENDERBUFFER, rboId);
        if (isMultisampled()) {
            glRenderbufferStorageMultisample(
                GL_RENDERBUFFER,
                samples,
                GL_DEPTH24_STENCIL8,
                width,
                height
            );
        } else {
            glRenderbufferStorage(
                GL_RENDERBUFFER,
                GL_DEPTH24_STENCIL8,
                width,
                height
            );
        }
        glFramebufferRenderbuffer(
            GL_FRAMEBUFFER,
            GL_DEPTH_STENCIL_ATTACHMENT,
            GL_RENDERBUFFER,
            rboId
        );
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glBindTexture(textureTarget, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        throw std::runtime_error("Framebuffer incomplete");
    }

    glBindTexture(textureTarget, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::setSamples(int sampleCount) {
    GLint maximumSamples = 1;
    glGetIntegerv(GL_MAX_SAMPLES, &maximumSamples);
    const int supportedSamples = std::clamp(sampleCount, 1, maximumSamples);
    if (samples == supportedSamples) {
        return;
    }

    samples = supportedSamples;
    recreateColorTexture();
    allocateAttachments();
}

void FrameBuffer::resolveTo(FrameBuffer &destination) const {
    if (!isMultisampled()) {
        throw std::logic_error("Resolve source must be multisampled");
    }
    if (destination.isMultisampled()) {
        throw std::logic_error("Resolve destination must be single-sampled");
    }
    if (width != destination.width || height != destination.height) {
        throw std::invalid_argument("Resolve framebuffers must have matching extents");
    }

    GLint previousReadFramebuffer = 0;
    GLint previousDrawFramebuffer = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFramebuffer);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, id);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, destination.id);
    glBlitFramebuffer(
        0, 0, width, height,
        0, 0, destination.width, destination.height,
        GL_COLOR_BUFFER_BIT,
        GL_NEAREST
    );

    glBindFramebuffer(GL_READ_FRAMEBUFFER, previousReadFramebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousDrawFramebuffer);
}

void FrameBuffer::resize(RenderExtent extent) {
    if (extent.width <= 0 || extent.height <= 0) {
        return;
    }
    if (width == extent.width && height == extent.height) {
        return;
    }

    width = extent.width;
    height = extent.height;
    allocateAttachments();
}

void FrameBuffer::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    glViewport(0, 0, width, height);
}

void FrameBuffer::bindDefault(RenderExtent windowExtent) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, windowExtent.width, windowExtent.height);
}

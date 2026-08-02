#include "FrameBuffer.h"

#include <stdexcept>

FrameBuffer::FrameBuffer(RenderExtent extent)
    : width(extent.width), height(extent.height) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("Framebuffer extent must be positive");
    }

    glGenFramebuffers(1, &id);
    glGenTextures(1, &textureId);
    glGenRenderbuffers(1, &rboId);

    try {
        allocateAttachments();
    } catch (...) {
        glDeleteFramebuffers(1, &id);
        glDeleteTextures(1, &textureId);
        glDeleteRenderbuffers(1, &rboId);
        throw;
    }
}

FrameBuffer::~FrameBuffer() {
    glDeleteFramebuffers(1, &id);
    glDeleteTextures(1, &textureId);
    glDeleteRenderbuffers(1, &rboId);
}

void FrameBuffer::allocateAttachments() {
    glBindFramebuffer(GL_FRAMEBUFFER, id);

    glBindTexture(GL_TEXTURE_2D, textureId);
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

    glBindRenderbuffer(GL_RENDERBUFFER, rboId);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        textureId,
        0
    );
    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        GL_DEPTH_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER,
        rboId
    );

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        throw std::runtime_error("Framebuffer incomplete");
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

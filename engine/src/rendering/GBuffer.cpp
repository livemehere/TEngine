#include "GBuffer.h"

#include <stdexcept>

namespace {
    void allocateColorTexture(
        const GLuint texture,
        const int width,
        const int height,
        const GLenum attachment
    ) {
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA16F,
            width,
            height,
            0,
            GL_RGBA,
            GL_FLOAT,
            nullptr
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            attachment,
            GL_TEXTURE_2D,
            texture,
            0
        );
    }
}

GBuffer::GBuffer(const RenderExtent extent)
    : width(extent.width),
      height(extent.height) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("G-buffer extent must be positive");
    }

    glGenFramebuffers(1, &id);
    glGenTextures(1, &positionTexture);
    glGenTextures(1, &normalTexture);
    glGenTextures(1, &albedoSpecTexture);
    glGenTextures(1, &depthStencilTexture);

    try {
        allocateAttachments();
    } catch (...) {
        glDeleteFramebuffers(1, &id);
        glDeleteTextures(1, &positionTexture);
        glDeleteTextures(1, &normalTexture);
        glDeleteTextures(1, &albedoSpecTexture);
        glDeleteTextures(1, &depthStencilTexture);
        throw;
    }
}

GBuffer::~GBuffer() {
    glDeleteFramebuffers(1, &id);
    glDeleteTextures(1, &positionTexture);
    glDeleteTextures(1, &normalTexture);
    glDeleteTextures(1, &albedoSpecTexture);
    glDeleteTextures(1, &depthStencilTexture);
}

void GBuffer::allocateAttachments() {
    GLint previousDrawFramebuffer = 0;
    GLint previousReadFramebuffer = 0;
    GLint previousTexture = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFramebuffer);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);

    glBindFramebuffer(GL_FRAMEBUFFER, id);
    allocateColorTexture(
        positionTexture,
        width,
        height,
        GL_COLOR_ATTACHMENT0
    );
    allocateColorTexture(
        normalTexture,
        width,
        height,
        GL_COLOR_ATTACHMENT1
    );
    allocateColorTexture(
        albedoSpecTexture,
        width,
        height,
        GL_COLOR_ATTACHMENT2
    );

    constexpr GLenum attachments[] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2
    };
    glDrawBuffers(3, attachments);

    glBindTexture(GL_TEXTURE_2D, depthStencilTexture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_DEPTH24_STENCIL8,
        width,
        height,
        0,
        GL_DEPTH_STENCIL,
        GL_UNSIGNED_INT_24_8,
        nullptr
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_DEPTH_STENCIL_ATTACHMENT,
        GL_TEXTURE_2D,
        depthStencilTexture,
        0
    );

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glBindTexture(GL_TEXTURE_2D, previousTexture);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousDrawFramebuffer);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, previousReadFramebuffer);
        throw std::runtime_error("G-buffer incomplete");
    }

    glBindTexture(GL_TEXTURE_2D, previousTexture);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousDrawFramebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, previousReadFramebuffer);
}

void GBuffer::resize(const RenderExtent extent) {
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

void GBuffer::bindForGeometry() const {
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    glViewport(0, 0, width, height);
}

void GBuffer::clear() const {
    constexpr GLfloat zero[] = {0.0f, 0.0f, 0.0f, 0.0f};
    glClearBufferfv(GL_COLOR, 0, zero);
    glClearBufferfv(GL_COLOR, 1, zero);
    glClearBufferfv(GL_COLOR, 2, zero);
    glClearBufferfi(GL_DEPTH_STENCIL, 0, 1.0f, 0);
}

void GBuffer::bindTextures(
    const GLuint positionSlot,
    const GLuint normalSlot,
    const GLuint albedoSpecSlot
) const {
    glActiveTexture(GL_TEXTURE0 + positionSlot);
    glBindTexture(GL_TEXTURE_2D, positionTexture);
    glActiveTexture(GL_TEXTURE0 + normalSlot);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glActiveTexture(GL_TEXTURE0 + albedoSpecSlot);
    glBindTexture(GL_TEXTURE_2D, albedoSpecTexture);
}

void GBuffer::blitDepthStencilTo(
    const GLuint destination,
    const RenderExtent extent
) const {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, id);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, destination);
    glBlitFramebuffer(
        0,
        0,
        width,
        height,
        0,
        0,
        extent.width,
        extent.height,
        GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
        GL_NEAREST
    );
}

#include "ShadowMap.h"

#include <algorithm>
#include <stdexcept>

ShadowMap::ShadowMap(const int initialResolution)
    : resolution(std::max(1, initialResolution)) {
    glGenFramebuffers(1, &framebuffer);
    glGenTextures(1, &depthTexture);

    try {
        allocate();
    } catch (...) {
        glDeleteTextures(1, &depthTexture);
        glDeleteFramebuffers(1, &framebuffer);
        throw;
    }
}

ShadowMap::~ShadowMap() {
    if (depthTexture != 0) {
        glDeleteTextures(1, &depthTexture);
    }
    if (framebuffer != 0) {
        glDeleteFramebuffers(1, &framebuffer);
    }
}

void ShadowMap::allocate() {
    GLint maximumTextureSize = 1;
    GLint previousTexture = 0;
    GLint previousDrawFramebuffer = 0;
    GLint previousReadFramebuffer = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximumTextureSize);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFramebuffer);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);
    resolution = std::clamp(resolution, 1, maximumTextureSize);

    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_DEPTH_COMPONENT24,
        resolution,
        resolution,
        0,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        nullptr
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    constexpr float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D,
        depthTexture,
        0
    );
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glBindTexture(GL_TEXTURE_2D, previousTexture);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousDrawFramebuffer);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, previousReadFramebuffer);
        throw std::runtime_error("Shadow map framebuffer incomplete");
    }

    glBindTexture(GL_TEXTURE_2D, previousTexture);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousDrawFramebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, previousReadFramebuffer);
}

void ShadowMap::resize(const int newResolution) {
    GLint maximumTextureSize = 1;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximumTextureSize);
    const int supportedResolution = std::clamp(
        newResolution,
        1,
        maximumTextureSize
    );
    if (resolution == supportedResolution) {
        return;
    }

    resolution = supportedResolution;
    allocate();
}

void ShadowMap::bindForWriting() const {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, resolution, resolution);
}

void ShadowMap::bindTexture(const GLuint slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
}

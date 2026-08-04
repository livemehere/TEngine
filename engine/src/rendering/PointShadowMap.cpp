#include "PointShadowMap.h"

#include <algorithm>
#include <stdexcept>

PointShadowMap::PointShadowMap(const int initialResolution)
    : resolution(std::max(1, initialResolution)) {
    glGenFramebuffers(1, &framebuffer);
    glGenTextures(1, &depthCubemap);

    try {
        allocate();
    } catch (...) {
        glDeleteTextures(1, &depthCubemap);
        glDeleteFramebuffers(1, &framebuffer);
        throw;
    }
}

PointShadowMap::~PointShadowMap() {
    if (depthCubemap != 0) {
        glDeleteTextures(1, &depthCubemap);
    }
    if (framebuffer != 0) {
        glDeleteFramebuffers(1, &framebuffer);
    }
}

void PointShadowMap::allocate() {
    GLint maximumTextureSize = 1;
    GLint previousTexture = 0;
    GLint previousDrawFramebuffer = 0;
    GLint previousReadFramebuffer = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximumTextureSize);
    glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &previousTexture);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFramebuffer);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);
    resolution = std::clamp(resolution, 1, maximumTextureSize);

    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    for (GLuint face = 0; face < 6; ++face) {
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
            0,
            GL_DEPTH_COMPONENT24,
            resolution,
            resolution,
            0,
            GL_DEPTH_COMPONENT,
            GL_FLOAT,
            nullptr
        );
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        depthCubemap,
        0
    );
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glBindTexture(GL_TEXTURE_CUBE_MAP, previousTexture);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousDrawFramebuffer);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, previousReadFramebuffer);
        throw std::runtime_error("Point shadow framebuffer incomplete");
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, previousTexture);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousDrawFramebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, previousReadFramebuffer);
}

void PointShadowMap::resize(const int newResolution) {
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

void PointShadowMap::bindForWriting() const {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, resolution, resolution);
}

void PointShadowMap::bindTexture(const GLuint slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
}

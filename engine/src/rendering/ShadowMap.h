#pragma once

#include <glad/glad.h>

class ShadowMap {
    GLuint framebuffer = 0;
    GLuint depthTexture = 0;
    int resolution = 0;

    void allocate();

public:
    explicit ShadowMap(int initialResolution);
    ~ShadowMap();

    ShadowMap(const ShadowMap &) = delete;
    ShadowMap &operator=(const ShadowMap &) = delete;

    void resize(int newResolution);
    void bindForWriting() const;
    void bindTexture(GLuint slot) const;

    [[nodiscard]] int getResolution() const { return resolution; }
    [[nodiscard]] GLuint getTextureId() const { return depthTexture; }
};

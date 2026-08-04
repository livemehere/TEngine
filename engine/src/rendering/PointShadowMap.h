#pragma once

#include <glad/glad.h>

class PointShadowMap {
    GLuint framebuffer = 0;
    GLuint depthCubemap = 0;
    int resolution = 0;

    void allocate();

public:
    explicit PointShadowMap(int initialResolution);
    ~PointShadowMap();

    PointShadowMap(const PointShadowMap &) = delete;
    PointShadowMap &operator=(const PointShadowMap &) = delete;

    void resize(int newResolution);
    void bindForWriting() const;
    void bindTexture(GLuint slot) const;

    [[nodiscard]] int getResolution() const { return resolution; }
    [[nodiscard]] GLuint getTextureId() const { return depthCubemap; }
};

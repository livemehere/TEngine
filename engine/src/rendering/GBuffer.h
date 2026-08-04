#pragma once

#include <glad/glad.h>

#include "RenderExtent.h"

class GBuffer {
    GLuint id = 0;
    GLuint positionTexture = 0;
    GLuint normalTexture = 0;
    GLuint albedoSpecTexture = 0;
    GLuint depthStencilRBO = 0;
    int width = 0;
    int height = 0;

    void allocateAttachments();

public:
    explicit GBuffer(RenderExtent extent);
    ~GBuffer();

    GBuffer(const GBuffer&) = delete;
    GBuffer& operator=(const GBuffer&) = delete;

    void resize(RenderExtent extent);
    void bindForGeometry() const;
    void clear() const;
    void bindTextures(
        GLuint positionSlot,
        GLuint normalSlot,
        GLuint albedoSpecSlot
    ) const;
    void blitDepthStencilTo(GLuint destination, RenderExtent extent) const;
};

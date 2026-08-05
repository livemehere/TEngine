#pragma once

#include <glad/glad.h>

#include "../graphics/FrameBuffer.h"
#include "RenderSettings.h"

class ResourceManager;
class Shader;

struct FrameBufferDebugInput {
    FrameBufferDebugView mode = FrameBufferDebugView::Off;
    GLuint texture2D = 0;
    GLuint cubeTexture = 0;
    RenderExtent extent{1, 1};
    float cameraNear = 0.1f;
    float cameraFar = 1000.0f;
    float depthRangeNear = 0.0f;
    float depthRangeFar = 30.0f;
    bool orthographic = false;
    int cubeFace = 0;
};

class FrameBufferDebugRenderer {
    GLuint vao = 0;
    const Shader &shader;
    FrameBuffer output;
    bool available = false;

public:
    explicit FrameBufferDebugRenderer(ResourceManager &resourceManager);
    ~FrameBufferDebugRenderer();

    FrameBufferDebugRenderer(const FrameBufferDebugRenderer &) = delete;
    FrameBufferDebugRenderer &operator=(const FrameBufferDebugRenderer &) =
            delete;

    void reset() noexcept { available = false; }
    bool render(const FrameBufferDebugInput &input);

    [[nodiscard]] GLuint getTextureId() const {
        return available ? output.getTextureId() : 0;
    }
};

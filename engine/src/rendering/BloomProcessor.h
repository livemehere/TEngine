#pragma once

#include <glad/glad.h>

#include "../graphics/FrameBuffer.h"

class ResourceManager;
class Shader;
class GpuProfiler;
struct RenderSettings;

class BloomProcessor {
    GLuint vao = 0;
    const Shader& extractShader;
    const Shader& blurShader;
    FrameBuffer brightBuffer;
    FrameBuffer blurBufferA;
    FrameBuffer blurBufferB;
    GpuProfiler &gpuProfiler;

public:
    BloomProcessor(ResourceManager& resourceManager, GpuProfiler &gpuProfiler);
    ~BloomProcessor();

    BloomProcessor(const BloomProcessor&) = delete;
    BloomProcessor& operator=(const BloomProcessor&) = delete;

    const FrameBuffer* process(
        const FrameBuffer& source,
        const RenderSettings& settings
    );
};

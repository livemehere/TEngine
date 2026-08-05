#pragma once

#include <glad/glad.h>

class FrameBuffer;
class ResourceManager;
class Shader;
class GpuProfiler;
struct RenderSettings;

class PostProcessor {
    GLuint vao = 0;
    const Shader& shader;
    GpuProfiler &gpuProfiler;

public:
    PostProcessor(ResourceManager& resourceManager, GpuProfiler &gpuProfiler);
    ~PostProcessor();

    PostProcessor(const PostProcessor&) = delete;
    PostProcessor& operator=(const PostProcessor&) = delete;

    void render(
        const FrameBuffer& source,
        const FrameBuffer* bloom,
        FrameBuffer& destination,
        const RenderSettings& settings
    );
};

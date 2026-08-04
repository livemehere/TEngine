#pragma once

#include <glad/glad.h>

class FrameBuffer;
class ResourceManager;
class Shader;
struct RenderSettings;

class PostProcessor {
    GLuint vao = 0;
    const Shader& shader;

public:
    PostProcessor(ResourceManager& resourceManager);
    ~PostProcessor();

    PostProcessor(const PostProcessor&) = delete;
    PostProcessor& operator=(const PostProcessor&) = delete;

    void render(
        const FrameBuffer& source,
        FrameBuffer& destination,
        const RenderSettings& settings
    );
};

#pragma once

#include <glad/glad.h>

#include "../resources/ResourceManager.h"


class PostProcessor {
    GLuint vao;
    const Shader& shader;

public:
    PostProcessor(ResourceManager& resourceManager);
    ~PostProcessor();

    void render(GLuint textureId);
};

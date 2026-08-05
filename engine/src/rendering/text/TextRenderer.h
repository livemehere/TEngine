#pragma once

#include <cstddef>
#include <vector>

#include <glad/glad.h>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "../RenderExtent.h"

class Font;
class ResourceManager;
class Scene;
class Shader;

class TextRenderer {
    struct Vertex {
        glm::vec3 position;
        glm::vec2 texCoord;
        glm::vec4 color;
    };

    const Font &font;
    const Shader &shader;
    GLuint vao = 0;
    GLuint vbo = 0;
    std::size_t bufferCapacity = 0;

    void appendText(
        std::vector<Vertex> &vertices,
        const char *text,
        const glm::vec3 &origin,
        const glm::vec3 &right,
        const glm::vec3 &up,
        float scale,
        const glm::vec4 &color,
        bool centered
    ) const;
    bool draw(
        const std::vector<Vertex> &vertices,
        const glm::mat4 &viewProjection,
        bool depthTest
    );

public:
    explicit TextRenderer(ResourceManager &resources);
    ~TextRenderer();

    TextRenderer(const TextRenderer &) = delete;
    TextRenderer &operator=(const TextRenderer &) = delete;

    bool renderWorld(
        const Scene &scene,
        const glm::mat4 &view,
        const glm::mat4 &projection
    );
    bool renderCanvas(const Scene &scene, RenderExtent extent);
};

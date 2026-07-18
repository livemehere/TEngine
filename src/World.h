#pragma once

#include <vector>
#include <glad/glad.h>

#include <glm/glm.hpp>

#include "renderer/Mesh.h"
#include "graphics/Shader.h"
#include "graphics/Texture2D.h"
#include "renderer/RenderObject.h"

constexpr std::array<uint8_t,4> pixels = {
    255,255,255,255,
};

class World {
private:
    Mesh quadMesh{
         std::array{
             Vertex{{ -0.5f, -0.5f, 0.0f}, {0.0f,0.0f}}, // left bottom
             Vertex{{ 0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}}, // right bottom
             Vertex{{ -0.5f, 0.5f, 0.0f}, {0.0f, 1.0f}}, // left top
             Vertex{{ 0.5f, 0.5f, 0.0f}, {1.0f, 1.0f}}, // right top
         },
        std::array<GLuint, 6>{
            0, 1, 2,
            1, 3, 2
         }
    };

    Shader shader{"shaders/basic.vert", "shaders/basic.frag"};
    Texture2D texture{1,1,pixels};

    std::vector<RenderObject> objects;

public:
    World();
    ~World();

    void update(const glm::mat4& view, const glm::mat4& projection);
    void render();
};

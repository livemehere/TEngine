#pragma once

#include <vector>
#include <glad/glad.h>

#include <glm/glm.hpp>

#include "opengl/Mesh.h"
#include "opengl/ShaderProgram.h"
#include "opengl/Texture2D.h"




constexpr std::array<uint8_t,16> pixels = {
    255,0,0,80,
    0,255,0,80,
    0,0,255,80,
    80,80,80,80,
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
        std::array{
            0, 1, 2,
            1, 3, 2
         }
    };

    // model
    GLuint modelLocation;
    glm::mat4 model;
    Transform transform {
        .position = {0.0f,0.0f,0.0f},
        .rotation = {0.0f,0.0f,0.0f},
        .scale = {1.0f,1.0f,1.0f},
    };

    // view
    GLuint viewLocation;

    // projection
    GLuint projectionLocation;
    glm::mat4 projection;


    ShaderProgram shader{"shaders/basic.vert", "shaders/basic.frag"};
    Texture2D texture{2,2,pixels };

public:
    World();
    ~World();

    void update(const glm::mat4& view, const glm::mat4& projection);
    void render();
};

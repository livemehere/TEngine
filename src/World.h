#pragma once

#include <vector>
#include <glad/glad.h>

#include <glm/glm.hpp>

struct Position {
    float x;
    float y;
    float z;
};

struct TexCoord {
   float u;
   float v;
};

struct Vertex {
    Position position;
    TexCoord texCoord;
};

class World {
private:
   std::vector<Vertex> vertices = {
       {{ -0.5f, -0.5f, 0.0f}, {0.0f,0.0f}}, // left bottom
       {{ 0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}}, // right bottom
       {{ -0.5f, 0.5f, 0.0f}, {0.0f, 1.0f}}, // left top
       {{ 0.5f, 0.5f, 0.0f}, {1.0f, 1.0f}}, // right top
   };
   std::vector<GLuint> indices = {
       0, 1, 2,
       1, 3, 2
   };

    int textureW = 2;
    int textureH = 2;
    std::vector<uint8_t> pixel = {
        255,0,0,80,
        0,255,0,80,
        0,0,255,80,
        80,80,80,80,
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

    GLuint VAO;
    GLuint VBO;
    GLuint EBO;
    GLuint program;
    GLuint texture;

public:
    World();
    ~World();

    void update(const glm::mat4& view, const glm::mat4& projection);
    void render();
};

#pragma once

#include <vector>
#include <glad/glad.h>

struct Position {
    float x;
    float y;
    float z;
};

struct UVCoord {
   float u;
    float v;
};

struct Vertex {
    Position position;
    UVCoord uvCoord;
};

class World {
private:
   std::vector<Vertex> vertices = {
       {{ -1.0f, -1.0f, 0.0f}, {0.0f,0.0f}}, // left bottom
       {{ 1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}}, // right bottom
       {{ -1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}, // left top
       {{ 1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}}, // right top
   };
   std::vector<GLuint> indices = {
       0, 1, 2,
       1, 3, 2
   };

    int textureW = 2;
    int textureH = 2;
    std::vector<uint8_t> pixel = {
        255,0,0,255,
        0,255,0,255,
        0,0,255,255,
        80,80,80,255,
    };

    GLuint VAO;
    GLuint VBO;
    GLuint EBO;
    GLuint program;
    GLuint texture;

public:
    World();
    ~World();

    void update();
    void render();
};

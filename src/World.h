#pragma once

#include <vector>
#include <glad/glad.h>

struct Position {
    float x;
    float y;
    float z;
};

struct Vertex {
    Position position;
};

class World {
private:
   std::vector<Vertex> vertices = {
       {{ -1.0f, -1.0f, 0.0f}}, // left bottom
       {{ 1.0f, -1.0f, 0.0f}}, // right bottom
       {{ -1.0f, 1.0f, 0.0f}}, // left top
       {{ 1.0f, 1.0f, 0.0f}}, // right top
   };
   std::vector<GLuint> indices = {
       0, 1, 2,
       1, 3, 2
   };

    GLuint VAO;
    GLuint VBO;
    GLuint EBO;
    GLuint program;

public:
    World();
    ~World();

    void update();
    void render();
};

#pragma once
#include <span>
#include <glad/glad.h>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec2 texCoord;
};

class Mesh {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLsizei indexCount;
public:
    Mesh(std::span<const Vertex> vertices, std::span<const int> indices);
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    void draw() const;

};

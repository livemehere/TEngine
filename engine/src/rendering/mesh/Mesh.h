#pragma once
#include <span>
#include <vector>
#include <glad/glad.h>
#include "./Vertex.h"

struct MeshBounds {
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};
};

class Mesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei indexCount = 0;
    std::vector<glm::vec3> positions;
    std::vector<GLuint> indices;
    MeshBounds bounds;
public:
    Mesh(std::span<const Vertex> vertices, std::span<const GLuint> indices);
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    void draw() const;
    void drawInstanced(GLuint instanceBuffer, GLsizei instanceCount) const;

    [[nodiscard]] GLsizei getTriangleCount() const {
        return indexCount / 3;
    }

    [[nodiscard]] const std::vector<glm::vec3> &getPositions() const {
        return positions;
    }

    [[nodiscard]] const std::vector<GLuint> &getIndices() const {
        return indices;
    }

    [[nodiscard]] const MeshBounds &getBounds() const { return bounds; }

};

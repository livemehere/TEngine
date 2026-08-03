#include "Mesh.h"

#include <glm/vec4.hpp>

namespace {
    constexpr GLuint InstanceMatrixLocation = 3;

    void setIdentityInstanceAttributes() {
        glVertexAttrib4f(InstanceMatrixLocation + 0, 1.0f, 0.0f, 0.0f, 0.0f);
        glVertexAttrib4f(InstanceMatrixLocation + 1, 0.0f, 1.0f, 0.0f, 0.0f);
        glVertexAttrib4f(InstanceMatrixLocation + 2, 0.0f, 0.0f, 1.0f, 0.0f);
        glVertexAttrib4f(InstanceMatrixLocation + 3, 0.0f, 0.0f, 0.0f, 1.0f);
    }
}

Mesh::Mesh(std::span<const Vertex> vertices, std::span<const GLuint> indices) : indexCount(static_cast<GLsizei>(indices.size())){
    // VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // VBO
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    // EBO
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    // layout
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(2);

    setIdentityInstanceAttributes();
}

Mesh::~Mesh() {
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteVertexArrays(1, &vao);
}

void Mesh::draw() const {
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount,GL_UNSIGNED_INT,0);
}

void Mesh::drawInstanced(
    GLuint instanceBuffer,
    GLsizei instanceCount
) const {
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);

    constexpr GLsizei stride = static_cast<GLsizei>(sizeof(glm::mat4));
    for (GLuint column = 0; column < 4; ++column) {
        const GLuint location = InstanceMatrixLocation + column;
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(
            location,
            4,
            GL_FLOAT,
            GL_FALSE,
            stride,
            reinterpret_cast<void *>(sizeof(glm::vec4) * column)
        );
        glVertexAttribDivisor(location, 1);
    }

    glDrawElementsInstanced(
        GL_TRIANGLES,
        indexCount,
        GL_UNSIGNED_INT,
        nullptr,
        instanceCount
    );

    for (GLuint column = 0; column < 4; ++column) {
        const GLuint location = InstanceMatrixLocation + column;
        glDisableVertexAttribArray(location);
        glVertexAttribDivisor(location, 0);
    }
    setIdentityInstanceAttributes();
}

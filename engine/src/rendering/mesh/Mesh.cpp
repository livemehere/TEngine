#include "Mesh.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <glm/vec4.hpp>

namespace {
    constexpr GLuint InstanceMatrixLocation = 3;
    constexpr GLuint TangentLocation = 7;

    glm::vec3 fallbackTangent(const glm::vec3 &normal) {
        const glm::vec3 reference =
                std::abs(normal.z) < 0.999f
                    ? glm::vec3(0.0f, 0.0f, 1.0f)
                    : glm::vec3(0.0f, 1.0f, 0.0f);
        return glm::normalize(glm::cross(reference, normal));
    }

    void generateMissingTangents(
        std::vector<Vertex> &vertices,
        std::span<const GLuint> indices
    ) {
        std::vector<glm::vec3> tangentSums(vertices.size(), glm::vec3(0.0f));
        std::vector<glm::vec3> bitangentSums(vertices.size(), glm::vec3(0.0f));

        for (std::size_t index = 0; index + 2 < indices.size(); index += 3) {
            const GLuint i0 = indices[index];
            const GLuint i1 = indices[index + 1];
            const GLuint i2 = indices[index + 2];
            if (i0 >= vertices.size() ||
                i1 >= vertices.size() ||
                i2 >= vertices.size()) {
                continue;
            }

            const Vertex &v0 = vertices[i0];
            const Vertex &v1 = vertices[i1];
            const Vertex &v2 = vertices[i2];
            const glm::vec3 edge1 = v1.position - v0.position;
            const glm::vec3 edge2 = v2.position - v0.position;
            const glm::vec2 deltaUv1 = v1.texCoord - v0.texCoord;
            const glm::vec2 deltaUv2 = v2.texCoord - v0.texCoord;
            const float determinant =
                    deltaUv1.x * deltaUv2.y -
                    deltaUv1.y * deltaUv2.x;
            if (std::abs(determinant) <= 0.000001f) {
                continue;
            }

            const float inverseDeterminant = 1.0f / determinant;
            const glm::vec3 tangent = inverseDeterminant * (
                deltaUv2.y * edge1 - deltaUv1.y * edge2
            );
            const glm::vec3 bitangent = inverseDeterminant * (
                -deltaUv2.x * edge1 + deltaUv1.x * edge2
            );

            for (const GLuint vertexIndex : {i0, i1, i2}) {
                tangentSums[vertexIndex] += tangent;
                bitangentSums[vertexIndex] += bitangent;
            }
        }

        for (std::size_t index = 0; index < vertices.size(); ++index) {
            Vertex &vertex = vertices[index];
            glm::vec3 normal = vertex.normal;
            if (glm::dot(normal, normal) <= 0.000001f) {
                normal = {0.0f, 1.0f, 0.0f};
            } else {
                normal = glm::normalize(normal);
            }

            glm::vec3 tangent = glm::vec3(vertex.tangent);
            if (glm::dot(tangent, tangent) <= 0.000001f) {
                tangent = tangentSums[index];
            }
            tangent -= normal * glm::dot(normal, tangent);
            if (glm::dot(tangent, tangent) <= 0.000001f) {
                tangent = fallbackTangent(normal);
            } else {
                tangent = glm::normalize(tangent);
            }

            float handedness = vertex.tangent.w;
            if (glm::dot(bitangentSums[index], bitangentSums[index]) >
                0.000001f) {
                handedness =
                        glm::dot(
                            glm::cross(normal, tangent),
                            bitangentSums[index]
                        ) < 0.0f
                            ? -1.0f
                            : 1.0f;
            }
            vertex.tangent = glm::vec4(tangent, handedness);
        }
    }

    void setIdentityInstanceAttributes() {
        glVertexAttrib4f(InstanceMatrixLocation + 0, 1.0f, 0.0f, 0.0f, 0.0f);
        glVertexAttrib4f(InstanceMatrixLocation + 1, 0.0f, 1.0f, 0.0f, 0.0f);
        glVertexAttrib4f(InstanceMatrixLocation + 2, 0.0f, 0.0f, 1.0f, 0.0f);
        glVertexAttrib4f(InstanceMatrixLocation + 3, 0.0f, 0.0f, 0.0f, 1.0f);
    }
}

Mesh::Mesh(std::span<const Vertex> vertices, std::span<const GLuint> sourceIndices)
    : indexCount(static_cast<GLsizei>(sourceIndices.size())),
      indices(sourceIndices.begin(), sourceIndices.end()) {
    positions.reserve(vertices.size());
    if (!vertices.empty()) {
        bounds.min = glm::vec3(std::numeric_limits<float>::max());
        bounds.max = glm::vec3(std::numeric_limits<float>::lowest());
        for (const Vertex &vertex : vertices) {
            positions.push_back(vertex.position);
            bounds.min = glm::min(bounds.min, vertex.position);
            bounds.max = glm::max(bounds.max, vertex.position);
        }
    }

    std::vector<Vertex> uploadVertices(vertices.begin(), vertices.end());
    generateMissingTangents(uploadVertices, sourceIndices);

    // VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // VBO
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        uploadVertices.size() * sizeof(Vertex),
        uploadVertices.data(),
        GL_STATIC_DRAW
    );

    // EBO
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sourceIndices.size() * sizeof(GLuint), sourceIndices.data(), GL_STATIC_DRAW);

    // layout
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(
        TangentLocation,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void *>(offsetof(Vertex, tangent))
    );
    glEnableVertexAttribArray(TangentLocation);

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

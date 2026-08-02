#pragma once
#include <glad/glad.h>

#include "Vertex.h"

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
};

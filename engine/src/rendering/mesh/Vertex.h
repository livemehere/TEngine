#pragma once

#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    // xyz: tangent direction, w: bitangent handedness.
    glm::vec4 tangent{0.0f, 0.0f, 0.0f, 1.0f};
};

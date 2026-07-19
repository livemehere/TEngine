#pragma once
#include <glm/vec3.hpp>

struct AmbientLight {
    glm::vec3 color{1.0f};
    float intensity{0.5f};
};

struct PointLight {
    glm::vec3 position{0.0f};
    float range{10.0f};

    glm::vec3 color{1.0f};
    float intensity{1.0f};
};
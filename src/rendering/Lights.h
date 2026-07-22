#pragma once
#include <glm/vec3.hpp>

struct AmbientLight {
    glm::vec3 color{1.0f};
    float intensity{0.1f};
};

struct PointLight {
    glm::vec3 position{0.0f};
    float range{1.0f};

    glm::vec3 color{1.0f};
    float intensity{1.0f};
};

struct DirectionalLight {
    glm::vec3 direction{0.0f, -1.0f, 0.0f};
    glm::vec3 color{1.0f};
    float intensity{1.0f};
};

struct SpotLight {
    glm::vec3 direction{0.0f, -1.0f, 0.0f};

    glm::vec3 position{0.0f};
    float range{10.0f};

    glm::vec3 color{1.0f};
    float intensity{1.0f};

    float innerAngle{12.5f};
    float outerAngle{17.5f};
};
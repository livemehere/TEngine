#pragma once

#include <glm/vec3.hpp>

#include "../scene/Component.h"

class AmbientLightComponent final : public Component {
public:
    glm::vec3 color{1.0f};
    float intensity{0.1f};
};

class PointLightComponent final : public Component {
public:
    float range{1.0f};
    glm::vec3 color{1.0f};
    float intensity{1.0f};
    bool castShadows = true;
};

class DirectionalLightComponent final : public Component {
public:
    glm::vec3 color{1.0f};
    float intensity{1.0f};
    bool castShadows = true;
};

class SpotLightComponent final : public Component {
public:
    float range{10.0f};
    glm::vec3 color{1.0f};
    float intensity{1.0f};
    float innerAngle{12.5f};
    float outerAngle{17.5f};
};

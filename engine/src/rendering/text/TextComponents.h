#pragma once

#include <string>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "../../scene/Component.h"

class WorldTextComponent final : public Component {
public:
    std::string text = "Text";
    glm::vec4 color{1.0f};
    glm::vec3 localOffset{0.0f};
    float scale = 0.01f;
    bool centered = true;
    bool depthTest = true;
};

class CanvasTextComponent final : public Component {
public:
    std::string text = "Text";
    glm::vec4 color{1.0f};
    glm::vec2 position{24.0f, 24.0f};
    float scale = 1.0f;
};

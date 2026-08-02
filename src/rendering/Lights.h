#pragma once

#include <glm/vec3.hpp>

#include "../scene/Component.h"

class AmbientLightComponent final : public Component {
public:
    glm::vec3 color{1.0f};
    float intensity{0.1f};

    [[nodiscard]] std::unique_ptr<Component> clone() const override {
        auto result = std::make_unique<AmbientLightComponent>();
        result->enabled = enabled;
        result->color = color;
        result->intensity = intensity;
        return result;
    }
};

class PointLightComponent final : public Component {
public:
    float range{1.0f};
    glm::vec3 color{1.0f};
    float intensity{1.0f};

    [[nodiscard]] std::unique_ptr<Component> clone() const override {
        auto result = std::make_unique<PointLightComponent>();
        result->enabled = enabled;
        result->range = range;
        result->color = color;
        result->intensity = intensity;
        return result;
    }
};

class DirectionalLightComponent final : public Component {
public:
    glm::vec3 color{1.0f};
    float intensity{1.0f};

    [[nodiscard]] std::unique_ptr<Component> clone() const override {
        auto result = std::make_unique<DirectionalLightComponent>();
        result->enabled = enabled;
        result->color = color;
        result->intensity = intensity;
        return result;
    }
};

class SpotLightComponent final : public Component {
public:
    float range{10.0f};
    glm::vec3 color{1.0f};
    float intensity{1.0f};
    float innerAngle{12.5f};
    float outerAngle{17.5f};

    [[nodiscard]] std::unique_ptr<Component> clone() const override {
        auto result = std::make_unique<SpotLightComponent>();
        result->enabled = enabled;
        result->range = range;
        result->color = color;
        result->intensity = intensity;
        result->innerAngle = innerAngle;
        result->outerAngle = outerAngle;
        return result;
    }
};

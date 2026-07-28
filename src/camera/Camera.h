#pragma once

#include <variant>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "../common.h"
#include "../core/Window.h"
#include "../rendering/Transform.h"

struct OrthoGraphicProjection {
    float height = 5.0f;
    float near = 0.1f;
    float far = 1000.0f;
};

struct PerspectiveProjection {
    float fov = 45.0f;
    float near = 0.1f;
    float far = 1000.0f;
};

using Projection = std::variant<OrthoGraphicProjection, PerspectiveProjection>;

class Camera {
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;

public:
    Transform transform {
        .position = {0.0f,0.0f,1.0f},
        .rotation = {0.0f,0.0f,0.0f},
        .scale = {1.0f,1.0f,1.0f},
    };
    Projection projection = PerspectiveProjection{};


    Camera() = default;
    ~Camera() = default;

    void update(float dt);
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(const WindowSize& size);

    glm::quat getOrientation() const {
        return glm::quat(glm::radians(transform.rotation));
    }
    glm::vec3 getForward() const {
        return getOrientation() * glm::vec3(0.0f, 0.0f, -1.0f);
    }
    glm::vec3 getRight() const {
        return getOrientation() * glm::vec3(1.0f, 0.0f, 0.0f);
    }

    void lookAt(const glm::vec3& target);
};

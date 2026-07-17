#pragma once

#include <variant>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "../common.h"
#include "../Window.h"

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
private:
    glm::mat4 viewMatrix_;
    glm::mat4 projectionMatrix_;
    glm::vec3 viewForward_{0.0f, 0.0f, -1.0f};
    glm::vec3 viewRight_{1.0f, 0.0f, 0.0f};
public:
    Transform transform {
        .position = {0.0f,0.0f,1.0f},
        .rotation = {0.0f,0.0f,0.0f},
        .scale = {1.0f,1.0f,1.0f},
    };
    Projection projection = PerspectiveProjection{};


    Camera() = default;
    ~Camera() = default;

    void update();
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(const WindowSize& size);

    glm::vec3 getForward() const {
        return viewForward_;
    }
    glm::vec3 getRight() const {
        return viewRight_;
    }
};

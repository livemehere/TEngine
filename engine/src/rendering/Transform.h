#pragma once

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

struct Transform {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 rotation{0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};

    [[nodiscard]] glm::quat getOrientation() const {
        return glm::quat(glm::radians(rotation));
    }

    [[nodiscard]] glm::vec3 getForward() const {
        return getOrientation() * glm::vec3(0.0f, 0.0f, -1.0f);
    }

    [[nodiscard]] glm::vec3 getRight() const {
        return getOrientation() * glm::vec3(1.0f, 0.0f, 0.0f);
    }

    void lookAt(const glm::vec3 &target) {
        const glm::vec3 direction = target - position;
        if (glm::dot(direction, direction) <= 1e-8f) {
            return;
        }

        constexpr glm::vec3 up{0.0f, 1.0f, 0.0f};
        const glm::quat orientation = glm::quatLookAt(glm::normalize(direction), up);
        rotation = glm::degrees(glm::eulerAngles(orientation));
    }

    glm::mat4 getLocalMatrix() const {
        glm::mat4 model(1.0f);

        model = glm::translate(model, position);

        glm::mat4 quatMatrix = glm::mat4_cast(getOrientation());
        model *= quatMatrix;

        model = glm::scale(model, scale);

        return model;
    }

    static bool decompose(const glm::mat4& matrix, Transform& transform) {
        glm::vec3 position;
        glm::quat orientation;
        glm::vec3 scale;

        /* not use */
        glm::vec3 skew;
        glm::vec4 perspective;

        const bool success = glm::decompose(
            matrix,
            scale,
            orientation,
            position,
            skew,
            perspective
        );

        if (!success) {
            return false;
        }

        orientation = glm::normalize(orientation);

        transform.position = position;
        transform.scale = scale;
        transform.rotation = glm::degrees(
            glm::eulerAngles(orientation)
        );

        return true;
    }
};

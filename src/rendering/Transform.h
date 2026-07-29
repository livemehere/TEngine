#pragma once
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

struct Transform {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    glm::mat4 getLocalMatrix() const {
        glm::mat4 model(1.0f);

        model = glm::translate(model, position);

        glm::mat4 quatMatrix = glm::mat4_cast(glm::quat(glm::radians(rotation)));
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

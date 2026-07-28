#pragma once
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

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
};

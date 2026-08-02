#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void Camera::update(float dt) {
    viewMatrix = glm::mat4(1.0f);

    // T
    viewMatrix = glm::translate(viewMatrix, transform.position);

    // R
    glm::quat viewQuaternion = getOrientation();
    glm::mat4 quaternionMatrix = glm::mat4_cast(viewQuaternion);
    viewMatrix *= quaternionMatrix;

    // inverse
    viewMatrix = glm::inverse(viewMatrix);
}


glm::mat4 Camera::getViewMatrix() const {
    return viewMatrix;
}

glm::mat4 Camera::getProjectionMatrix(const RenderExtent &size) {
    // if (size.w == 0 || size.h == 0) return projectionMatrix;

    if (std::holds_alternative<PerspectiveProjection>(projection)) {
        const auto &perspective = std::get<PerspectiveProjection>(projection);
        projectionMatrix = glm::perspective(glm::radians(perspective.fov),
                                            static_cast<float>(size.width) / static_cast<float>(size.height),
                                            perspective.near, perspective.far);
    } else {
        const auto &orthographic = std::get<OrthoGraphicProjection>(projection);
        const float ratio = static_cast<float>(size.width) / static_cast<float>(size.height);
        const float halfHeight = orthographic.height / 2.0f;
        const float halfWidth = ratio * halfHeight;
        projectionMatrix = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, orthographic.near,
                                      orthographic.far);
    }
    return projectionMatrix;
}

void Camera::lookAt(const glm::vec3 &target) {
    glm::vec3 dir = target - transform.position;
    dir = glm::normalize(dir);

    constexpr glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::quat rotationQuat = glm::quatLookAt(dir, up);
    transform.rotation = glm::degrees(glm::eulerAngles(rotationQuat));
}

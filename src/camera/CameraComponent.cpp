#include "CameraComponent.h"

#include <glm/gtc/matrix_transform.hpp>

glm::mat4 CameraComponent::getProjectionMatrix(const RenderExtent &size) const {
    if (size.width <= 0 || size.height <= 0) {
        return glm::mat4(1.0f);
    }

    if (std::holds_alternative<PerspectiveProjection>(projection)) {
        const auto &perspective = std::get<PerspectiveProjection>(projection);
        return glm::perspective(
            glm::radians(perspective.fov),
            static_cast<float>(size.width) / static_cast<float>(size.height),
            perspective.near,
            perspective.far
        );
    }

    const auto &orthographic = std::get<OrthoGraphicProjection>(projection);
    const float ratio = static_cast<float>(size.width) / static_cast<float>(size.height);
    const float halfHeight = orthographic.height / 2.0f;
    const float halfWidth = ratio * halfHeight;
    return glm::ortho(
        -halfWidth,
        halfWidth,
        -halfHeight,
        halfHeight,
        orthographic.near,
        orthographic.far
    );
}

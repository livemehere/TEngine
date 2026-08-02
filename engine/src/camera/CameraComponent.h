#pragma once

#include <variant>

#include <glm/glm.hpp>

#include "../rendering/RenderExtent.h"
#include "../scene/Component.h"

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

class CameraComponent final : public Component {
public:
    Projection projection = PerspectiveProjection{};

    [[nodiscard]] glm::mat4 getProjectionMatrix(const RenderExtent& size) const;
};

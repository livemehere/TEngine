#pragma once

#include <optional>

#include <glm/vec2.hpp>

#include "../rendering/RenderExtent.h"

class SceneViewport {
    glm::vec2 framebufferOrigin{0.0f};
    RenderExtent framebufferExtent{};

public:
    void set(glm::vec2 origin, RenderExtent extent) {
        framebufferOrigin = origin;
        framebufferExtent = extent;
    }

    [[nodiscard]] const RenderExtent &getExtent() const {
        return framebufferExtent;
    }

    [[nodiscard]] std::optional<glm::vec2> windowToLocal(
        glm::vec2 windowPosition
    ) const {
        const glm::vec2 local = windowPosition - framebufferOrigin;
        if (local.x < 0.0f || local.y < 0.0f ||
            local.x >= static_cast<float>(framebufferExtent.width) ||
            local.y >= static_cast<float>(framebufferExtent.height)) {
            return std::nullopt;
        }
        return local;
    }

    [[nodiscard]] glm::vec2 localToNdc(glm::vec2 localPosition) const {
        return {
            localPosition.x / static_cast<float>(framebufferExtent.width) * 2.0f - 1.0f,
            1.0f - localPosition.y / static_cast<float>(framebufferExtent.height) * 2.0f
        };
    }
};

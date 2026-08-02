#pragma once

#include "Component.h"
#include "../rendering/Transform.h"

class TransformComponent final : public Component {
public:
    Transform local;

    [[nodiscard]] std::unique_ptr<Component> clone() const override {
        auto result = std::make_unique<TransformComponent>();
        result->enabled = enabled;
        result->local = local;
        return result;
    }
};

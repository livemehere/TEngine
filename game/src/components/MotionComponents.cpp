#include "MotionComponents.h"

#include <cmath>

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>

#include <scene/Entity.h>
#include <scene/Scene.h>
#include <scene/TransformComponent.h>

namespace {
Transform *findLocalTransform(Behaviour &behaviour) {
    Entity *entity = behaviour.getScene().findEntity(behaviour.getEntityId());
    if (!entity) {
        return nullptr;
    }
    return &entity->getComponent<TransformComponent>().local;
}
}

void SpinComponent::update(float dt) {
    if (Transform *transform = findLocalTransform(*this)) {
        transform->rotation += degreesPerSecond * dt;
    }
}

void BobComponent::start() {
    elapsedTime_ = 0.0f;
    if (const Transform *transform = findLocalTransform(*this)) {
        startHeight_ = transform->position.y;
    }
}

void BobComponent::update(float dt) {
    Transform *transform = findLocalTransform(*this);
    if (!transform) {
        return;
    }

    elapsedTime_ += dt;
    const float phase = elapsedTime_ * frequency * glm::two_pi<float>();
    transform->position.y = startHeight_ + std::sin(phase) * amplitude;
}

void OrbitComponent::start() {
    angle_ = 0.0f;

    const Transform *transform = findLocalTransform(*this);
    if (!transform) {
        return;
    }

    startPosition_ = transform->position;
    const glm::vec2 offset{
        startPosition_.x - center.x,
        startPosition_.z - center.z
    };
    radius_ = glm::length(offset);
    if (radius_ > 0.0001f) {
        angle_ = std::atan2(offset.y, offset.x);
    }
}

void OrbitComponent::update(float dt) {
    Transform *transform = findLocalTransform(*this);
    if (!transform || radius_ <= 0.0001f) {
        return;
    }

    angle_ += glm::radians(degreesPerSecond) * dt;
    transform->position = {
        center.x + std::cos(angle_) * radius_,
        startPosition_.y,
        center.z + std::sin(angle_) * radius_
    };
}

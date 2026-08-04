#pragma once

#include <glm/vec3.hpp>

#include <scene/Behaviour.h>

class SpinComponent final : public Behaviour {
public:
    glm::vec3 degreesPerSecond{0.0f, 45.0f, 0.0f};

protected:
    void update(float dt) override;
};

class BobComponent final : public Behaviour {
    float startHeight_ = 0.0f;
    float elapsedTime_ = 0.0f;

public:
    float amplitude = 0.6f;
    float frequency = 0.5f;

protected:
    void start() override;
    void update(float dt) override;
};

class OrbitComponent final : public Behaviour {
    glm::vec3 startPosition_{0.0f};
    float radius_ = 0.0f;
    float angle_ = 0.0f;

public:
    glm::vec3 center{0.0f};
    float degreesPerSecond = 30.0f;

protected:
    void start() override;
    void update(float dt) override;
};

#pragma once
#include <scene/Behaviour.h>


class TestComponent final : public Behaviour {
public:
    float speed = 1.0f;

    TestComponent() = default;

protected:
    void update(float dt) override;
};

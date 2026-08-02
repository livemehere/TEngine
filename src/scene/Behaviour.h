#pragma once

#include "Component.h"

class Scene;

class Behaviour : public Component {
    friend class Scene;

    bool started_ = false;

protected:
    virtual void start() {}
    virtual void update(float dt) {}
    virtual void lateUpdate(float dt) {}

public:
    [[nodiscard]] bool hasStarted() const noexcept { return started_; }
};

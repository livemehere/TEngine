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
    virtual void stop() {}

public:
    // Concrete behaviours must clone their editable state so Play mode can run
    // in an isolated Scene without modifying the editor Scene.
    [[nodiscard]] virtual std::unique_ptr<Component> clone() const override = 0;
    [[nodiscard]] bool hasStarted() const noexcept { return started_; }
};

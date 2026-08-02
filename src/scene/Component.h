#pragma once

#include "EntityId.h"

class Entity;
class Scene;

class Component {
    friend class Entity;

    Scene* scene_ = nullptr;
    EntityId entityId_ = 0;

    void attachTo(Scene& scene, EntityId entityId) noexcept {
        scene_ = &scene;
        entityId_ = entityId;
    }

    void detachFromEntity() noexcept {
        scene_ = nullptr;
        entityId_ = 0;
    }

protected:
    virtual void onAttach() noexcept {}
    virtual void onDetach() noexcept {}

public:
    bool enabled = true;

    Component() = default;
    virtual ~Component() = default;

    Component(const Component&) = delete;
    Component& operator=(const Component&) = delete;
    Component(Component&&) = delete;
    Component& operator=(Component&&) = delete;

    [[nodiscard]] Scene& getScene() { return *scene_; }
    [[nodiscard]] const Scene& getScene() const { return *scene_; }
    [[nodiscard]] EntityId getEntityId() const noexcept { return entityId_; }
};

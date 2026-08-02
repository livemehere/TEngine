#pragma once

#include "EntityId.h"

class Entity;
class Scene;
class SceneSerializer;

class Component {
    friend class Entity;
    friend class Scene;
    friend class SceneSerializer;

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
    Component(const Component& other) noexcept
        : enabled(other.enabled) {}

    virtual void onAttach() noexcept {}
    virtual void onDetach() noexcept {}

public:
    bool enabled = true;

    Component() = default;
    virtual ~Component() = default;

    Component& operator=(const Component&) = delete;
    Component(Component&&) = delete;
    Component& operator=(Component&&) = delete;

    [[nodiscard]] Scene& getScene() { return *scene_; }
    [[nodiscard]] const Scene& getScene() const { return *scene_; }
    [[nodiscard]] EntityId getEntityId() const noexcept { return entityId_; }
};

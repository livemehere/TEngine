#pragma once

#include <concepts>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>

#include "Component.h"
#include "EntityId.h"
#include "TransformComponent.h"
#include "../rendering/mesh/MeshRendererComponent.h"
#include "../rendering/skybox/SkyboxComponent.h"

class Scene;

class Entity {
    Scene *scene_;
    std::unordered_map<std::type_index, std::unique_ptr<Component> > components_;

    void clearComponents() noexcept {
        for (auto &[type, component]: components_) {
            component->onDetach();
            component->detachFromEntity();
        }
        components_.clear();
    }

public:
    EntityId id;
    std::string name = "Entity";

    std::optional<EntityId> parentId = std::nullopt;
    size_t siblingIndex = 0;

    std::optional<MeshRendererComponent> meshRenderComponent;
    std::optional<SkyboxComponent> skyboxComponent;

    Entity(Scene &scene, EntityId id, std::string name, size_t siblingIndex)
        : id(id), name(std::move(name)), siblingIndex(siblingIndex), scene_(&scene) {
        addComponent<TransformComponent>();
    }
    ~Entity() { clearComponents(); }

    Entity(const Entity &) = delete;
    Entity &operator=(const Entity &) = delete;
    Entity(Entity &&) noexcept = default;
    Entity &operator=(Entity &&other) noexcept {
        if (this == &other) {
            return *this;
        }

        clearComponents();

        id = other.id;
        name = std::move(other.name);
        parentId = other.parentId;
        siblingIndex = other.siblingIndex;
        meshRenderComponent = std::move(other.meshRenderComponent);
        skyboxComponent = std::move(other.skyboxComponent);
        scene_ = other.scene_;
        components_ = std::move(other.components_);

        return *this;
    }

    template<typename T, typename... Args>
        requires std::derived_from<T, Component>
    T &addComponent(Args &&... args) {
        const std::type_index type = typeid(T);
        if (components_.contains(type)) {
            throw std::logic_error("Entity already has this component type");
        }

        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        T &result = *component;
        Component &base = result;
        base.attachTo(*scene_, id);
        components_.emplace(type, std::move(component));
        base.onAttach();
        return result;
    }

    template<typename T>
        requires std::derived_from<T, Component>
    [[nodiscard]] bool hasComponent() const {
        return components_.contains(std::type_index(typeid(T)));
    }

    template<typename T>
        requires std::derived_from<T, Component>
    T *tryGetComponent() {
        const auto it = components_.find(std::type_index(typeid(T)));
        return it == components_.end() ? nullptr : static_cast<T *>(it->second.get());
    }

    template<typename T>
        requires std::derived_from<T, Component>
    const T *tryGetComponent() const {
        const auto it = components_.find(std::type_index(typeid(T)));
        return it == components_.end() ? nullptr : static_cast<const T *>(it->second.get());
    }

    template<typename T>
        requires std::derived_from<T, Component>
    T &getComponent() {
        T *component = tryGetComponent<T>();
        if (!component) {
            throw std::logic_error("Entity does not have this component type");
        }
        return *component;
    }

    template<typename T>
        requires std::derived_from<T, Component>
    const T &getComponent() const {
        const T *component = tryGetComponent<T>();
        if (!component) {
            throw std::logic_error("Entity does not have this component type");
        }
        return *component;
    }

    template<typename T>
        requires std::derived_from<T, Component>
    bool removeComponent() noexcept {
        static_assert(
            !std::same_as<T, TransformComponent>,
            "TransformComponent is required and cannot be removed"
        );

        const auto it = components_.find(std::type_index(typeid(T)));
        if (it == components_.end()) {
            return false;
        }

        it->second->onDetach();
        it->second->detachFromEntity();
        components_.erase(it);
        return true;
    }
};

struct EntityMoveRequest {
    EntityId sourceId;
    // std::nullopt = Root
    std::optional<EntityId> newParentId;
    size_t insertIndex;
};

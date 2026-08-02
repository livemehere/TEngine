#pragma once

#include <concepts>
#include <deque>
#include <functional>
#include <vector>

#include "Entity.h"
#include "../camera/CameraComponent.h"
#include "../rendering/model/Model.h"

class Scene {
    friend class SceneSerializer;

    EntityId entitySeq = 0;
    std::deque<Entity> entities;
    mutable size_t componentIterationDepth = 0;
    std::optional<EntityId> activeCameraId;

    void endComponentIteration() noexcept;
    void flushPendingComponentChanges() noexcept;

    class ComponentIterationScope {
        Scene &scene;

    public:
        explicit ComponentIterationScope(Scene &scene) : scene(scene) {
            ++scene.componentIterationDepth;
        }

        ~ComponentIterationScope() {
            scene.endComponentIteration();
        }

        ComponentIterationScope(const ComponentIterationScope &) = delete;
        ComponentIterationScope &operator=(const ComponentIterationScope &) = delete;
    };

    bool wouldCreateCycle(EntityId childId, EntityId parentId);

public:
    Scene() = default;
    ~Scene();

    void updateEditor(float dt);
    void updateRuntime(float dt);
    void stopRuntime();

    Entity& createEntity(const std::string& name);

    bool setActiveCamera(EntityId entityId);
    [[nodiscard]] std::optional<EntityId> getActiveCameraId() const;
    [[nodiscard]] Entity* getActiveCameraEntity();
    [[nodiscard]] const Entity* getActiveCameraEntity() const;

    template<typename... Components, typename Function>
        requires (std::derived_from<Components, Component> && ...)
    void each(Function&& function) {
        static_assert(sizeof...(Components) > 0, "Scene::each requires at least one component type");
        constexpr bool acceptsEntity =
                std::invocable<Function&, Entity&, Components&...>;
        constexpr bool acceptsComponents =
                std::invocable<Function&, Components&...>;
        static_assert(
            acceptsEntity || acceptsComponents,
            "Scene::each callback has an incompatible parameter list"
        );

        ComponentIterationScope iterationScope(*this);
        const size_t entityCount = entities.size();
        for (size_t index = 0; index < entityCount; ++index) {
            Entity& entity = entities[index];
            if ((entity.hasComponent<Components>() && ...)) {
                if constexpr (acceptsEntity) {
                    std::invoke(function, entity, entity.getComponent<Components>()...);
                } else {
                    std::invoke(function, entity.getComponent<Components>()...);
                }
            }
        }
    }

    template<typename... Components, typename Function>
        requires (std::derived_from<Components, Component> && ...)
    void each(Function&& function) const {
        static_assert(sizeof...(Components) > 0, "Scene::each requires at least one component type");
        constexpr bool acceptsEntity =
                std::invocable<Function&, const Entity&, const Components&...>;
        constexpr bool acceptsComponents =
                std::invocable<Function&, const Components&...>;
        static_assert(
            acceptsEntity || acceptsComponents,
            "Scene::each callback has an incompatible parameter list"
        );

        ComponentIterationScope iterationScope(const_cast<Scene &>(*this));
        const size_t entityCount = entities.size();
        for (size_t index = 0; index < entityCount; ++index) {
            const Entity& entity = entities[index];
            if ((entity.hasComponent<Components>() && ...)) {
                if constexpr (acceptsEntity) {
                    std::invoke(function, entity, entity.getComponent<Components>()...);
                } else {
                    std::invoke(function, entity.getComponent<Components>()...);
                }
            }
        }
    }

    Entity* findEntity(EntityId id);
    const Entity* findEntity(EntityId id) const;

    std::vector<const Entity*> getChildren(std::optional<EntityId> id) const;
    std::vector<EntityId> getChildrenIds(std::optional<EntityId> id, std::optional<EntityId> excludeId) const;
    bool moveEntity(EntityId sourceId, std::optional<EntityId> newParentId, size_t insertIndex, bool keepLocalTransform = false);

    glm::mat4 getWorldMatrix(const Entity& entity) const;

    EntityId instantiateModel(const Model& model, const Material& fallbackMaterial, const std::string& name);
};

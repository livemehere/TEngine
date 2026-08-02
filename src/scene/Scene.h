#pragma once

#include <concepts>
#include <functional>
#include <vector>

#include "Entity.h"
#include "../camera/Camera.h"
#include "../rendering/model/Model.h"

class Scene {
    EntityId entitySeq = 0;
    std::vector<Entity> entities;

    bool wouldCreateCycle(EntityId childId, EntityId parentId);

public:
    Camera camera;

    Scene() = default;
    ~Scene();

    void update(float dt);

    Entity& createEntity(const std::string& name);

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

        for (Entity& entity: entities) {
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

        for (const Entity& entity: entities) {
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

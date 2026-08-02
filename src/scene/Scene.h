#pragma once

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
    const std::vector<Entity>& getEntities() const { return entities; }

    Entity* findEntity(EntityId id);
    const Entity* findEntity(EntityId id) const;

    std::vector<const Entity*> getChildren(std::optional<EntityId> id) const;
    std::vector<EntityId> getChildrenIds(std::optional<EntityId> id, std::optional<EntityId> excludeId) const;
    bool moveEntity(EntityId sourceId, std::optional<EntityId> newParentId, size_t insertIndex, bool keepLocalTransform = false);

    glm::mat4 getWorldMatrix(const Entity& entity) const;

    EntityId instantiateModel(const Model& model, const Material& fallbackMaterial, const std::string& name);
};

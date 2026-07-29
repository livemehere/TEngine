#pragma once

#include <vector>

#include "Entity.h"
#include "../camera/Camera.h"
#include "../rendering/Lights.h"

class Scene {
    EntityId entitySeq = 0;
    std::vector<Entity> entities;

    bool wouldCreateCycle(EntityId childId, EntityId parentId);

public:
    Camera camera;

    /* Lights */
    AmbientLight ambientLight;
    std::vector<DirectionalLight> directionalLights;
    std::vector<PointLight> pointLights;
    std::vector<SpotLight> spotLights;

    Scene() = default;
    ~Scene() = default;

    void update(float dt);

    Entity& createEntity(const std::string& name);
    const std::vector<Entity>& getEntities() const { return entities; }

    Entity* findEntity(EntityId id);
    const Entity* findEntity(EntityId id) const;

    std::vector<const Entity*> getChildren(std::optional<EntityId> id) const;
    bool setParent(EntityId childId, EntityId parentId);
    bool unsetParent(EntityId childId, EntityId parentId);

    glm::mat4 getWorldMatrix(const Entity& entity) const;
};

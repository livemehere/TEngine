#pragma once

#include <vector>

#include "Entity.h"
#include "../camera/Camera.h"
#include "../rendering/Lights.h"

class Scene {
    EntityId nextEntityId = 0;
    // QUESTION: why not use smart pointer?
    std::vector<Entity> entities;
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
};

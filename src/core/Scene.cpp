#include "Scene.h"

void Scene::update(float dt) {
    camera.update(dt);
}

Entity &Scene::createEntity(const std::string &name) {
    Entity &entity = entities.emplace_back();
    entity.id = entitySeq++;
    entity.name = name;
    return entity;
}

Entity *Scene::findEntity(const EntityId id) {
    auto it = std::find_if(entities.begin(), entities.end(), [id](const Entity &entity) {
        return entity.id == id;
    });

    if (it == entities.end()) {
        return nullptr;
    }

    return &(*it);
}

const Entity * Scene::findEntity(EntityId id) const {
    auto it = std::find_if(entities.begin(), entities.end(), [id](const Entity &entity) {
    return entity.id == id;
});

    if (it == entities.end()) {
        return nullptr;
    }

    return &(*it);
}

std::vector<const Entity *> Scene::getChildren(const EntityId id) {
    std::vector<const Entity*> children;
    for (const Entity& entity : entities) {
       if (entity.parent && *entity.parent == id) {
           children.push_back(&entity);
       }
    }
    return children;
}

glm::mat4 Scene::getWorldMatrix(const Entity &entity) const {
    glm::mat4 localMatrix = entity.localTransform.getLocalMatrix();
    if (!entity.parent) {
        return localMatrix;
    }

    const Entity* parentEntity = findEntity(*entity.parent);
    if (!parentEntity) {
        return localMatrix;
    }

    return getWorldMatrix(*parentEntity) * localMatrix;
}
